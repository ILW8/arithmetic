#pragma once

#include <memory>
#include <vector>
#include "inputnum.h"

class Progress
{
public:
    Progress() { }

    void add_stage(int cost) { _total_cost += cost; _costs.push_back(cost); }
    void next_stage() { _cur_stage++; update(0, 0); }
    void update(double progress, int op_count);
    void time_init(double elapsed);
    void set_parent(Progress* parent) { _parent = parent; }

    double progress_stage() { return _cur_progress; }
    double progress_total() { if (_cur_stage >= _costs.size()) return 1; int cost = 0; for (int i = 0; i < _cur_stage; cost += _costs[i], i++); return (cost + _costs[_cur_stage]*_cur_progress)/_total_cost; }
    int cost_total() { return _total_cost; }
    double time_total() { return _time_total + _time_stage; }
    double time_op() { return _time_op; }
    int op_count() { return _op_count; }
    int num_stages() { return (int)_costs.size(); }

private:
    std::vector<int> _costs;
    int _total_cost = 0;
    int _cur_stage = 0;
    double _cur_progress = 0;
    double _time_total = 0;
    double _time_stage = 0;
    double _time_op = 0;
    double _timer = 0;
    int _op_count = 0;
    Progress* _parent = nullptr;
};

class Logging
{
public:
    static const int LEVEL_DEBUG = 0;
    static const int LEVEL_INFO = 1;
    static const int LEVEL_WARNING = 2;
    static const int LEVEL_ERROR = 3;
    static const int LEVEL_RESULT = 4;

public:
    Logging(int level = LEVEL_INFO) : _level(level) { }
    virtual ~Logging() { }

    void debug(const char* message...);
    void info(const char* message...);
    void warning(const char* message...);
    void error(const char* message...);
    void result(const char* message...);

    virtual void report(const std::string& message, int level);
    virtual void report_progress();
    virtual void report_factor(InputNum& input, const arithmetic::Giant& f);
    virtual void report_result(const std::string& message);

    int level() { return _level; }
    Progress& progress() { return _progress; }
    const std::string& prefix() { return _prefix; }
    void set_prefix(const std::string& prefix) { _prefix = prefix; }

private:
    int _level;
    Progress _progress;
    std::string _prefix;
    bool _print_prefix = true;
};

class SubLogging : public Logging
{
public:
    SubLogging(Logging& parent, int level = LEVEL_WARNING) : Logging(level), _parent(parent) { progress().set_parent(&parent.progress()); }

    virtual void report(const std::string& message, int level) override { _parent.report(message, level); }
    virtual void report_factor(InputNum& input, const arithmetic::Giant& f) override { _parent.report_factor(input, f); }
    virtual void report_result(const std::string& message) override { }

private:
    Logging& _parent;
};
