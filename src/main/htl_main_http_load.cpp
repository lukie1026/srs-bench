/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <htl_stdinc.hpp>

#include <getopt.h>
#include <stdlib.h>

#include <string>
using namespace std;

// project lib
#include <htl_core_log.hpp>
#include <htl_core_error.hpp>
#include <htl_app_http_load.hpp>

#include <htl_main_utility.hpp>

#define DefaultDelaySeconds 0.0
#define DefaultHttpUrl "http://127.0.0.1:3080/hls/segm130813144315787-522881.ts"
#define DefaultHTTPUrl_I "http://127.0.0.1:8080/live/livestream_{i}.flv"

int discovery_options(int argc, char** argv, 
    bool& show_help, bool& show_version, string& url, int& threads, 
    double& startup, double& delay, double& error, double& report, int& count, int& stream
){
    int ret = ERROR_SUCCESS;
    
    static option long_options[] = {
        SharedOptions()
        {"stream", no_argument, 0, 'l'},
        {0, 0, 0, 0}
    };
    
    int opt = 0;
    int option_index = 0;
    while((opt = getopt_long(argc, argv, "hvc:r:t:s:d:e:m:l:", long_options, &option_index)) != -1){
        switch(opt){
            ProcessSharedOptions()
            case 'l':
                stream = atoi(optarg);
            break;
            default:
                show_help = true;
                break;
        }
    }
    
    // check values
    if(url == ""){
        show_help = true;
        return ret;
    }

    return ret;
}

void help(char** argv){
    printf("%s, Copyright (c) 2013-2015 winlin\n", ProductHTTPName);
    
    printf(""
        "Usage: %s <Options> <-u URL>\n"
        "%s base on st(state-threads), support huge concurrency.\n"
        "Options:\n"
        ShowHelpPart1()
        ShowHelpPart2()
        "\n"
        "Examples:\n"
        "1. start a client\n"
        "   %s -c 1 -r %s\n"
        "2. start 1000 clients\n"
        "   %s -c 1000 -r %s\n"
        "3. start 10000 clients\n"
        "   %s -c 10000 -r %s\n"
        "4. start 100000 clients\n"
        "   %s -c 100000 -r %s\n"
        "5. start 1000 clients, 100 streams\n"
        "   %s -c 1000 -l 100 -r %s\n"
        "\n"
        "This program built for %s.\n"
        "Report bugs to <%s>\n",
        argv[0], argv[0], 
        DefaultThread, DefaultHttpUrl, DefaultCount, // part1
        (double)DefaultStartupSeconds, DefaultDelaySeconds, // part2
        DefaultErrorSeconds, DefaultReportSeconds, // part2
        argv[0], DefaultHttpUrl, argv[0], DefaultHttpUrl, argv[0], DefaultHttpUrl, argv[0], DefaultHttpUrl, argv[0], DefaultHTTPUrl_I,
        BuildPlatform, BugReportEmail);
        
    exit(0);
}

int main(int argc, char** argv){
    int ret = ERROR_SUCCESS;
    int inc_flag = 1;
    
    bool show_help = false, show_version = false; 
    string url; int threads = DefaultThread; 
    double start = DefaultStartupSeconds, delay = DefaultDelaySeconds, error = DefaultErrorSeconds;
    double report = DefaultReportSeconds; int count = DefaultCount; int stream = 0;
    
    if((ret = discovery_options(argc, argv, show_help, show_version, url, threads, start, delay, error, report, count, stream)) != ERROR_SUCCESS){
        Error("discovery options failed. ret=%d", ret);
        return ret;
    }
    if (stream == 0) {
        inc_flag = 0;
        stream = threads;
    }
    Trace("params url=%s, threads=%d, start=%.2f, delay=%.2f, error=%.2f, report=%.2f, count=%d", 
        url.c_str(), threads, start, delay, error, report, count);
    
    if(show_help){
        help(argv);
    }
    if(show_version){
        version();
    }
    
    StFarm farm;
    
    if((ret = farm.Initialize(report)) != ERROR_SUCCESS){
        Error("initialize the farm failed. ret=%d", ret);
        return ret;
    }

    for(int i = 0; i < threads; i++){
        StHttpTask* task = new StHttpTask();

        int m = threads/stream;
        std::string http_url = url;
        if (inc_flag != 0) {
            char index[16];
            snprintf(index, sizeof(index), "%d", i/m);
            std::string _index = index;
            size_t pos = std::string::npos;
            if ((pos = http_url.find("{i}")) != std::string::npos) {
                http_url = http_url.replace(pos, 3, _index);
            }
        }

        if((ret = task->Initialize(http_url, start, delay, error, count)) != ERROR_SUCCESS){
            Error("initialize task failed, url=%s, ret=%d", http_url.c_str(), ret);
            return ret;
        }
        
        if((ret = farm.Spawn(task)) != ERROR_SUCCESS){
            Error("st farm spwan task failed, ret=%d", ret);
            return ret;
        }
    }
    
    farm.WaitAll();
    
    return 0;
}

