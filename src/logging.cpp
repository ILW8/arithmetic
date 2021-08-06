
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <iostream>
#include "gwnum.h"
#include "cpuid.h"
#include "logging.h"

void Progress::update(double progress, int op_count)
{
    _cur_progress = progress;
    if (_timer == 0)
        _timer = getHighResTimer();
    double elapsed = (getHighResTimer() - _timer)/getHighResTimerFrequency();
    _timer = getHighResTimer();
    _time_total += elapsed;
    if (_op_count < op_count)
        _time_op = elapsed/(op_count - _op_count);
    _op_count = op_count;
}

void Progress::time_init(double elapsed)
{
    _time_total = elapsed;
    _timer = getHighResTimer();
}

void Logging::debug(const char* message...)
{
    if (_level > LEVEL_DEBUG)
        return;
    char buf[1000];
    va_list args;
    va_start(args, message);
    vsnprintf(buf, 1000, message, args);
    va_end(args);
    report(buf, LEVEL_DEBUG);
}

void Logging::info(const char* message...)
{
    if (_level > LEVEL_INFO)
        return;
    char buf[1000];
    va_list args;
    va_start(args, message);
    vsnprintf(buf, 1000, message, args);
    va_end(args);
    report(buf, LEVEL_INFO);
}

void Logging::warning(const char* message...)
{
    if (_level > LEVEL_WARNING)
        return;
    char buf[1000];
    va_list args;
    va_start(args, message);
    vsnprintf(buf, 1000, message, args);
    va_end(args);
    report(buf, LEVEL_WARNING);
}

void Logging::error(const char* message...)
{
    if (_level > LEVEL_ERROR)
        return;
    char buf[1000];
    va_list args;
    va_start(args, message);
    vsnprintf(buf, 1000, message, args);
    va_end(args);
    report(buf, LEVEL_ERROR);
    report_result(buf);
}

void Logging::result(const char* message...)
{
    char buf[1000];
    va_list args;
    va_start(args, message);
    vsnprintf(buf, 1000, message, args);
    va_end(args);
    report_result(buf);
}

void Logging::report(const std::string& message, int level)
{
    if (_print_prefix)
        std::cout << _prefix;
    std::cout << message;
    _print_prefix = message.back() == '\n';
}

void Logging::report_progress()
{
    if (progress().num_stages() > 1)
        info("%.1f%% stage / %.1f%% total, time per op: %.6f ms.\n", progress().progress_stage()*100, progress().progress_total()*100, progress().time_op()*1000);
    else if (progress().num_stages() > 0)
        info("%.1f%% done, time per op: %.3f ms.\n", progress().progress_stage()*100, progress().time_op()*1000);
}

void Logging::report_factor(InputNum& input, const arithmetic::Giant& f)
{
    std::string str = f.to_string();
    warning("found factor %s\n", str.data());
    result("found factor %s, time: %.1f s.\n", str.data(), progress().time_total());
    FILE *fp = fopen("factors.txt", "a");
    if (fp)
    {
        fprintf(fp, "%s | %s\n", str.data(), input.input_text().data());
        fclose(fp);
    }
}

void Logging::report_result(const std::string& message)
{
    FILE *fp = fopen("result.txt", "a");
    if (fp)
    {
        fwrite(_prefix.data(), 1, _prefix.length(), fp);
        fwrite(message.data(), 1, message.length(), fp);
        fclose(fp);
    }
}
