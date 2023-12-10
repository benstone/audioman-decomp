/**
 * @brief AudioMan filter test
 **/
#include <Windows.h>
#include <AUDIOMAN.H>
#include <stdio.h>
#include <string.h>

/**
 * @brief Function that takes a sound and applies a filter to it.
 * If psndFiltered is not NULL you will need to call Release() on it when finished.
 **/
typedef HRESULT (*FilterFunc)(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[]);

typedef struct Filter_t
{
    const char *name;
    const char *description;
    FilterFunc func;
} Filter;

/**
 * @brief Log the result of calling a function that returns a HRESULT
 **/
void LogHR(const char *functionName, HRESULT hr)
{
    if (FAILED(hr))
    {
        printf("%s failed: 0x%lx\n", functionName, hr);
    }
    else
    {
        if (hr == S_OK)
        {
            printf("%s succeeded\n", functionName);
        }
        else
        {
            printf("%s succeeded: 0x%lx\n", functionName, hr);
        }
    }
}

// Create a bias filter
HRESULT BiasFilter(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[])
{
    HRESULT hr = AllocBiasFilter(psndFiltered, psndSrc);
    LogHR("AllocBiasFilter", hr);
    return hr;
}

// Create a trim filter
HRESULT TrimFilter(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[])
{
    HRESULT hr = AllocTrimFilter(psndFiltered, psndSrc);
    LogHR("AllocTrimFilter", hr);
    return hr;
}

// Create a gate filter
HRESULT GateFilter(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[])
{
    HRESULT hr = E_INVALIDARG;
    float threshold = 0;

    // Parse command-line args
    if (argc >= 1)
    {
        if (sscanf(argv[0], "%f", &threshold) == 1 && threshold < 0)
        {
            hr = S_OK;
        }
    }

    // Show usage if args are invalid
    if (hr == E_INVALIDARG)
    {
        printf("Gate filter args: <threshold-in-db>\n");
        printf("example: -60.0\n");
    }
    else
    {
        printf("Threshold: %.2f dB\n", threshold);
        hr = AllocGateFilter(psndFiltered, psndSrc, threshold);
        LogHR("AllocGateFilter", hr);
    }

    return hr;
}

// Create a cache filter
HRESULT CacheFilter(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[])
{
    HRESULT hr = S_OK;
    CACHECONFIG cacheConfig = {0};
    cacheConfig.dwSize = sizeof(cacheConfig);

    // Set options in cache config
    cacheConfig.dwCacheTime = 500;
    cacheConfig.fSrcFormat = TRUE;

    // Parse optional command-line args
    if (argc > 0)
    {
        hr = E_INVALIDARG;
        if (argc == 1)
        {
            if (sscanf(argv[0], "%lu", &cacheConfig.dwCacheTime) == 1)
            {
                hr = S_OK;
            }
        }
    }

    // Show usage if args are invalid
    if (hr == E_INVALIDARG)
    {
        printf("Cache filter args: <cache-time-ms>\n");
    }
    else
    {
        printf("Cache time: %ld ms\n", cacheConfig.dwCacheTime);
        hr = AllocCacheFilter(psndFiltered, psndSrc, &cacheConfig);
        LogHR("AllocCacheFilter", hr);
    }

    return hr;
}

// Create a loop filter
HRESULT LoopFilter(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[])
{
    HRESULT hr = E_INVALIDARG;
    DWORD loops = 0;

    // Parse command-line args
    if (argc >= 1)
    {
        if (sscanf(argv[0], "%ld", &loops) == 1 && loops != 0xFFFFFFFF)
        {
            hr = S_OK;
        }
    }

    // Show usage if args are invalid
    if (hr == E_INVALIDARG)
    {
        printf("Loop filter args: <number-of-times-to-repeat-sound>\n");
    }
    else
    {
        printf("Loops: %lu\n", loops);
        hr = AllocLoopFilter(psndFiltered, psndSrc, loops);
        LogHR("AllocLoopFilter", hr);
    }

    return hr;
}

// Create a clip filter
HRESULT ClipFilter(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[])
{
    HRESULT hr = E_INVALIDARG;
    DWORD start = 0, end = 0;

    // Parse command-line args
    if (argc == 2)
    {
        if (sscanf(argv[0], "%lu", &start) == 1 && sscanf(argv[1], "%lu", &end) == 1)
        {
            hr = S_OK;
        }
    }

    // Show usage if args are invalid
    if (hr == E_INVALIDARG)
    {
        printf("Clip filter args: <start-sample> <end-sample>\n");
    }
    else
    {
        printf("Clip: %lu - %lu\n", start, end);
        hr = AllocClipFilter(psndFiltered, psndSrc, start, end);
        LogHR("AllocClipFilter", hr);
    }

    return hr;
}

// Create a conversion filter
HRESULT ConvertFilter(LPSOUND psndSrc, LPSOUND *psndFiltered, int argc, char *argv[])
{
    HRESULT hr = S_OK;
    WAVEFORMATEX wfxDst = {0};

    // Get current wave format to use as the starting point
    hr = psndSrc->GetFormat(&wfxDst, sizeof(wfxDst));
    LogHR("GetFormat", hr);

    // Set output format to match defaults used by 3D Movie Maker
    if (SUCCEEDED(hr))
    {
        wfxDst.nSamplesPerSec = 11025;
        wfxDst.nAvgBytesPerSec = 11025;
        wfxDst.wBitsPerSample = 8;
        wfxDst.nChannels = 1;
        wfxDst.nBlockAlign = 1;

        hr = AllocConvertFilter(psndFiltered, psndSrc, &wfxDst);
        LogHR("AllocConvertFilter", hr);
    }

    return hr;
}

// Show command-line usage
void usage(const char *programName, const Filter *filters, const int numberOfFilters)
{
    printf("AudioMan filter demo\n");
    printf("Usage: %s <input-file> <output-file> <filter> [filter-args...]\n\n", programName);

    printf("Filters:\n");
    for (int i = 0; i < numberOfFilters; i++)
    {
        printf("%s - %s\n", filters[i].name, filters[i].description);
    }
}

// Entrypoint
int main(int argc, char *argv[])
{
    HRESULT hr = 0;
    LPSOUND psndSrc = NULL;
    LPSOUND psndFiltered = NULL;
    char *inputFile = NULL;
    char *outputFile = NULL;
    DWORD sourceSamples = 0;

    const Filter filters[] = {{"bias", "removes DC bias from sound", BiasFilter},
                              {"cache", "caches the sound in memory before writing it to disk", CacheFilter},
                              {"clip", "creates a clip from the sound", ClipFilter},
                              {"convert", "converts a sound to a different format", ConvertFilter},
                              {"gate", "attenuates sound below a given threshold", GateFilter},
                              {"loop", "repeats a sound", LoopFilter},
                              {"trim", "removes silence from start and end of sound", TrimFilter}};

    const int numberOfFilters = sizeof(filters) / sizeof(filters[0]);
    int selectedFilter = 0;

    // Parse arguments
    hr = E_INVALIDARG;
    if (argc >= 4)
    {
        inputFile = argv[1];
        outputFile = argv[2];

        // Find the selected filter by name
        for (int i = 0; i < numberOfFilters && hr != S_OK; i++)
        {
            if (_stricmp(argv[3], filters[i].name) == 0)
            {
                selectedFilter = i;
                hr = S_OK;
            }
        }
    }

    // Print usage if arguments are bad
    if (hr == E_INVALIDARG)
    {
        usage(argv[0], filters, numberOfFilters);
        return -1;
    }

    printf("Input: %s\n", inputFile);
    printf("Output: %s\n", outputFile);
    printf("Filter: %s\n", filters[selectedFilter].name);

    // Load the source file
    if (SUCCEEDED(hr))
    {
        hr = AllocSoundFromFile(&psndSrc, inputFile, 0, TRUE, NULL);
        LogHR("AllocSoundFromFile", hr);
    }

    // Get number of samples in the source file
    if (SUCCEEDED(hr))
    {
        sourceSamples = psndSrc->GetSamples();
        printf("Source Samples: %lu\n", sourceSamples);
    }

    // Add the filter
    if (SUCCEEDED(hr))
    {
        // Pass args to filter
        int filterArgc = 0;
        char **filterArgv = NULL;

        if (argc > 4)
        {
            filterArgc = argc - 4;
            filterArgv = argv + 4;
        }

        hr = filters[selectedFilter].func(psndSrc, &psndFiltered, filterArgc, filterArgv);
    }

    // Write the result to a file
    if (SUCCEEDED(hr))
    {
        DWORD filteredSamples = psndFiltered->GetSamples();
        if (filteredSamples != sourceSamples)
        {
            printf("Filtered Samples: %lu\n", filteredSamples);
        }

        hr = SoundToFileAsWave(psndFiltered, outputFile);
        LogHR("SoundToFileAsWave", hr);
    }

    // Cleanup
    if (psndFiltered != NULL)
        psndFiltered->Release();
    if (psndSrc != NULL)
        psndSrc->Release();

#ifdef _DEBUG
    DetectLeaks(TRUE, TRUE);
#endif

    return hr;
}