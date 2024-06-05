#include <filesystem>
#include <boost/algorithm/string/replace.hpp>
#include <chrono>
#include <climits>
#include <cmath>
#include <atomic>
#include <thread>
#include <mutex>

#include "SoundExporter.h"
#include "Util.h"
#include "Xcept.h"
#include "Constants.h"
#include "Debug.h"
#include "ConfigManager.h"
#include "MP2KContext.h"
#include "OS.h"

/*
 * public SoundExporter
 */

SoundExporter::SoundExporter(SongTable& songTable, bool benchmarkOnly, bool seperate)
: songTable(songTable), benchmarkOnly(benchmarkOnly), seperate(seperate)
{
}

void SoundExporter::Export(const std::vector<SongEntry>& entries)
{
    /* create directories for file export */
    std::filesystem::path dir = ConfigManager::Instance().GetWavOutputDir();
    if (std::filesystem::exists(dir)) {
        if (!std::filesystem::is_directory(dir)) {
            throw Xcept("Output directory exists but isn't a dir");
        }
    }
    else if (!std::filesystem::create_directories(dir)) {
        throw Xcept("Creating output directory failed");
    }

    /* setup export thread worker function */
    std::atomic<size_t> currentSong = 0;
    std::atomic<size_t> totalBlocksRendered = 0;

    std::function<void(void)> threadFunc = [&]() {
        OS::LowerThreadPriority();
        while (true) {
            size_t i = currentSong++;   // atomic ++
            if (i >= entries.size())
                return;

            std::string fname = entries[i].name;
            boost::replace_all(fname, "/", "_");
            Debug::print("{:3}% - Rendering to file: \"{}\"", (i + 1) * 100 / entries.size(), fname);
            const auto fileName = std::format("{}/{:03d} - {}", dir.string(), i + 1, fname);
            totalBlocksRendered += exportSong(fileName, entries[i].GetUID());
        }
    };

    /* run the actual export threads */
    auto startTime = std::chrono::high_resolution_clock::now();

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 1;
    std::vector<std::thread> workers;
    for (size_t i = 0; i < numThreads; i++)
        workers.emplace_back(threadFunc);
    for (auto& w : workers)
        w.join();
    workers.clear();

    auto endTime = std::chrono::high_resolution_clock::now();

    /* report finished progress */
    if (std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count() == 0) {
        Debug::print("Successfully wrote {} files", entries.size());
    } else {
        size_t secondsTotal = static_cast<size_t>(std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count());
        size_t blocksPerSecond = totalBlocksRendered / secondsTotal;
        Debug::print("Successfully wrote {} files at {} blocks per second ({} seconds total)", entries.size(), blocksPerSecond, secondsTotal);
    }
}

/*
 * private SoundExporter
 */

void SoundExporter::writeSilence(SNDFILE *ofile, double seconds)
{
    if (seconds <= 0.0)
        return;
    size_t samples = static_cast<size_t>(std::round(STREAM_SAMPLERATE * seconds));
    std::vector<sample> silence(samples, {0.0f, 0.0f});
    sf_writef_float(ofile, reinterpret_cast<float *>(silence.data()), static_cast<sf_count_t>(silence.size()));
}

size_t SoundExporter::exportSong(const std::filesystem::path& fileName, uint16_t uid)
{
    const auto &cm = ConfigManager::Instance();
    const auto &cfg = cm.GetCfg();

    const MP2KSoundMode mp2kSoundMode{
        cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq()
    };

    const AgbplaySoundMode agbplaySoundMode{
        .resamplerTypeNormal = cfg.GetResType(),
        .resamplerTypeFixed = cfg.GetResTypeFixed(),
        .reverbType = cfg.GetRevType(),
        .cgbPolyphony = cm.GetCgbPolyphony(),
        .dmaBufferLen = cfg.GetRevBufSize(),
        .trackLimit = cfg.GetTrackLimit(),
        .maxLoops = cm.GetMaxLoopsExport(),
        .padSilenceSecondsStart = cm.GetPadSecondsStart(),
        .padSilenceSecondsEnd = cm.GetPadSecondsEnd(),
        .accurateCh3Quantization = cfg.GetAccurateCh3Quantization(),
        .accurateCh3Volume = cfg.GetAccurateCh3Volume(),
        .emulateCgbSustainBug = cfg.GetSimulateCGBSustainBug(),
    };
    
    MP2KContext ctx(
        Rom::Instance(),
        mp2kSoundMode,
        agbplaySoundMode
    );

    ctx.InitSong(songTable.GetPosOfSong(uid));
    size_t blocksRendered = 0;
    size_t nBlocks = ctx.mixer.GetSamplesPerBuffer();
    size_t nTracks = ctx.player.tracks.size();
    double padSecondsStart = ConfigManager::Instance().GetPadSecondsStart();
    double padSecondsEnd = ConfigManager::Instance().GetPadSecondsEnd();

    if (!benchmarkOnly) 
    {
        /* save each track to a separate file */
        if (seperate)
        {
            std::vector<SNDFILE *> ofiles(nTracks, nullptr);
            std::vector<SF_INFO> oinfos(nTracks);

            for (size_t i = 0; i < nTracks; i++)
            {
                memset(&oinfos[i], 0, sizeof(oinfos[i]));
                oinfos[i].samplerate = STREAM_SAMPLERATE;
                oinfos[i].channels = 2; // stereo
                oinfos[i].format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
                const std::string outName = fmt::format("{}.{:02d}.wav", fileName.string(), i);
                ofiles[i] = sf_open(outName.c_str(), SFM_WRITE, &oinfos[i]);
                if (ofiles[i] == NULL)
                    Debug::print("Error: {}", sf_strerror(NULL));
            }

            while (true)
            {
                ctx.SoundMain();
                if (ctx.HasEnded())
                    break;

                assert(ctx.player.tracks.size() == nTracks);

                for (size_t i = 0; i < nTracks; i++) 
                {
                    // do not write to invalid files
                    if (ofiles[i] == NULL)
                        continue;
                    sf_count_t processed = 0;
                    do {
                        processed += sf_writef_float(ofiles[i], &ctx.masterAudioBuffer[processed].left, sf_count_t(nBlocks) - processed);
                    } while (processed < sf_count_t(nBlocks));
                }
                blocksRendered += nBlocks;
            }

            for (SNDFILE *& i : ofiles)
            {
                int err = sf_close(i);
                if (err != 0)
                    Debug::print("Error: {}", sf_error_number(err));
            }
        }
        else
        {
            SF_INFO oinfo;
            memset(&oinfo, 0, sizeof(oinfo));
            oinfo.samplerate = STREAM_SAMPLERATE;
            oinfo.channels = 2; // sterep
            oinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            SNDFILE *ofile = sf_open((fileName.string() + ".wav").c_str(), SFM_WRITE, &oinfo);
            if (ofile == NULL) {
                Debug::print("Error: {}", sf_strerror(NULL));
                return 0;
            }

            writeSilence(ofile, padSecondsStart);

            while (true) 
            {
                ctx.SoundMain();
                if (ctx.HasEnded())
                    break;

                sf_count_t processed = 0;
                do {
                    processed += sf_writef_float(ofile, &ctx.masterAudioBuffer[processed].left, sf_count_t(nBlocks) - processed);
                } while (processed < sf_count_t(nBlocks));
                blocksRendered += nBlocks;
            }

            writeSilence(ofile, padSecondsEnd);

            int err;
            if ((err = sf_close(ofile)) != 0)
                Debug::print("Error: {}", sf_error_number(err));
        }
    } 
    // if benchmark only
    else {
        while (true)
        {
            ctx.SoundMain();
            blocksRendered += nBlocks;
            if (ctx.HasEnded())
                break;
        }
    }
    return blocksRendered;
}
