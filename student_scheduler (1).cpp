#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <deque>
#include <algorithm>
#include <iomanip>
using namespace std;
typedef long long ll;
#define pb push_back
#define el "\n"

struct Resource
{
    int id;
    int total;
    int available;
};

struct Operation
{
    char kind;
    int a;
    int b;
};

struct Burst
{
    bool isCPU;
    vector<Operation> ops;
    int ioDuration;
};

struct Process
{
    int pid, arrival, pr, originalPr;
    vector<Burst> bursts;

    int burstInd = 0;
    int opInd = 0;
    int remC = 0;

    map<int, int> hold;
    int waitRid = -1;
    int waitAmt = 0;

    long long waitingTime = 0;
    int finishTime = -1;

    int state = 0;
    int qleft = 0;
    int ioRem = 0;
    int readyWait = 0;
    int starved = 0;

    long long totalCpu = 0;
};

struct GanttSeg
{
    int pid;
    int st;
    int en;
};

vector<Resource> resources;
vector<Process> procs;

void loadFile()
{
    string filename = "inputFile.txt";

    ifstream fin(filename);
    if (!fin.is_open())
    {
        cout << "Cannot open file" << "\n";
        return;
    }

    vector<string> lines;
    string line;

    while (getline(fin, line))
    {
        int i = 0, j = (int)line.size() - 1;
        while (i <= j && isspace(line[i]))
            i++;
        while (j >= i && isspace(line[j]))
            j--;
        string t = (i > j ? "" : line.substr(i, j - i + 1));
        if (!t.empty())
            lines.pb(t);
    }
    fin.close();

    if (lines.empty())
    {
        cout << "Empty file" << "\n";
        return;
    }

    string resLine = lines[0];
    for (int k = 0; k < (int)resLine.size(); k++)
    {
        if (resLine[k] == '[')
        {
            k++;
            while (k < (int)resLine.size() && isspace(resLine[k]))
                k++;

            int id = 0, inst = 0;

            while (k < (int)resLine.size() && isdigit(resLine[k]))
            {
                id = id * 10 + (resLine[k] - '0');
                k++;
            }

            while (k < (int)resLine.size() && resLine[k] != ',')
                k++;

            if (k < (int)resLine.size() && resLine[k] == ',')
                k++;

            while (k < (int)resLine.size() && isspace(resLine[k]))
                k++;
            while (k < (int)resLine.size() && isdigit(resLine[k]))
            {
                inst = inst * 10 + (resLine[k] - '0');
                k++;
            }

            while (k < (int)resLine.size() && resLine[k] != ']')
                k++;

            Resource r;
            r.id = id;
            r.total = inst;
            r.available = inst;
            resources.pb(r);
        }
    }

    for (int li = 1; li < (int)lines.size(); li++)
    {
        string s = lines[li];

        Process p;
        p.bursts.clear();

        stringstream ss(s);
        ss >> p.pid >> p.arrival >> p.pr;
        p.originalPr = p.pr;

        int start = -1;
        for (int i = 0; i < (int)s.size(); i++)
        {
            if (i + 2 < (int)s.size() && s.substr(i, 3) == "CPU")
            {
                start = i;
                break;
            }
            if (i + 1 < (int)s.size() && s.substr(i, 2) == "IO")
            {
                start = i;
                break;
            }
        }
        if (start == -1)
        {
            procs.pb(p);
            continue;
        }

        string rest = s.substr(start);
        int i = 0, n = (int)rest.size();

        while (i < n)
        {
            while (i < n && isspace(rest[i]))
                i++;
            if (i >= n)
                break;

            if (i + 2 < n && rest.substr(i, 3) == "CPU")
            {
                i += 3;
                while (i < n && rest[i] != '{')
                    i++;
                if (i >= n)
                    break;

                int openPos = i;

                int depth = 0, closePos = -1;
                for (int j = openPos; j < n; j++)
                {
                    if (rest[j] == '{')
                        depth++;
                    else if (rest[j] == '}')
                    {
                        depth--;
                        if (depth == 0)
                        {
                            closePos = j;
                            break;
                        }
                    }
                }
                if (closePos == -1)
                    break;

                string inside = rest.substr(openPos + 1, closePos - openPos - 1);

                vector<string> items;
                string cur = "";
                int brDepth = 0;

                for (int t = 0; t < (int)inside.size(); t++)
                {
                    char c = inside[t];
                    if (c == '[')
                        brDepth++;
                    if (c == ']')
                        brDepth--;

                    if (c == ',' && brDepth == 0)
                    {
                        int a = 0, b = (int)cur.size() - 1;
                        while (a <= b && isspace(cur[a]))
                            a++;
                        while (b >= a && isspace(cur[b]))
                            b--;
                        string tok = (a > b ? "" : cur.substr(a, b - a + 1));
                        if (!tok.empty())
                            items.pb(tok);
                        cur = "";
                    }
                    else
                        cur.push_back(c);
                }
                {
                    int a = 0, b = (int)cur.size() - 1;
                    while (a <= b && isspace(cur[a]))
                        a++;
                    while (b >= a && isspace(cur[b]))
                        b--;
                    string tok = (a > b ? "" : cur.substr(a, b - a + 1));
                    if (!tok.empty())
                        items.pb(tok);
                }

                Burst B;
                B.isCPU = true;
                B.ioDuration = 0;
                B.ops.clear();

                for (int x = 0; x < (int)items.size(); x++)
                {
                    string tok = items[x];

                    int a1 = 0, b1 = (int)tok.size() - 1;
                    while (a1 <= b1 && isspace(tok[a1]))
                        a1++;
                    while (b1 >= a1 && isspace(tok[b1]))
                        b1--;
                    tok = (a1 > b1 ? "" : tok.substr(a1, b1 - a1 + 1));
                    if (tok.empty())
                        continue;

                    Operation op;

                    if (tok[0] == 'R' || tok[0] == 'F')
                    {
                        int L = -1, Rr = -1;
                        for (int z = 0; z < (int)tok.size(); z++)
                            if (tok[z] == '[')
                            {
                                L = z;
                                break;
                            }
                        for (int z = 0; z < (int)tok.size(); z++)
                            if (tok[z] == ']')
                            {
                                Rr = z;
                                break;
                            }

                        string mid = tok.substr(L + 1, Rr - L - 1);

                        int comma = -1;
                        for (int z = 0; z < (int)mid.size(); z++)
                            if (mid[z] == ',')
                            {
                                comma = z;
                                break;
                            }

                        string s1 = mid.substr(0, comma);
                        string s2 = mid.substr(comma + 1);

                        int x1 = 0, y1 = (int)s1.size() - 1;
                        while (x1 <= y1 && isspace(s1[x1]))
                            x1++;
                        while (y1 >= x1 && isspace(s1[y1]))
                            y1--;
                        s1 = (x1 > y1 ? "" : s1.substr(x1, y1 - x1 + 1));

                        int x2 = 0, y2 = (int)s2.size() - 1;
                        while (x2 <= y2 && isspace(s2[x2]))
                            x2++;
                        while (y2 >= x2 && isspace(s2[y2]))
                            y2--;
                        s2 = (x2 > y2 ? "" : s2.substr(x2, y2 - x2 + 1));

                        int rid = stoll(s1);
                        int amt = stoll(s2);

                        op.kind = tok[0];
                        op.a = rid;
                        op.b = amt;
                    }
                    else
                    {
                        int dur = stoll(tok);
                        op.kind = 'C';
                        op.a = dur;
                        op.b = 0;
                    }

                    B.ops.pb(op);
                }

                p.bursts.pb(B);
                i = closePos + 1;
            }
            else if (i + 1 < n && rest.substr(i, 2) == "IO")
            {
                i += 2;
                while (i < n && rest[i] != '{')
                    i++;
                if (i >= n)
                    break;

                int openPos = i;

                int depth = 0, closePos = -1;
                for (int j = openPos; j < n; j++)
                {
                    if (rest[j] == '{')
                        depth++;
                    else if (rest[j] == '}')
                    {
                        depth--;
                        if (depth == 0)
                        {
                            closePos = j;
                            break;
                        }
                    }
                }
                if (closePos == -1)
                    break;

                string inside = rest.substr(openPos + 1, closePos - openPos - 1);

                int a = 0, b = (int)inside.size() - 1;
                while (a <= b && isspace(inside[a]))
                    a++;
                while (b >= a && isspace(inside[b]))
                    b--;
                inside = (a > b ? "" : inside.substr(a, b - a + 1));

                Burst B;
                B.isCPU = false;
                B.ops.clear();
                B.ioDuration = inside.empty() ? 0 : (int)stoll(inside);

                p.bursts.pb(B);
                i = closePos + 1;
            }
            else
                i++;
        }

        procs.pb(p);
    }
}

void solve()
{
    resources.clear();
    procs.clear();
    loadFile();

    // states
    const int NEW_ = 0, READY_ = 1, RUN_ = 2, IO_ = 3, DONE_ = 4, BLK_ = 5;

    int Q = 30;

    deque<int> rq[21];
    vector<int> ioList, blkList;

    // resource id -> index
    unordered_map<int,int> ridToIdx;
    for (int i=0;i<(int)resources.size();i++) ridToIdx[resources[i].id]=i;

    auto canGrant = [&](int rid, int amt)->bool{
        if (!ridToIdx.count(rid)) return false;
        return resources[ridToIdx[rid]].available >= amt;
    };
    auto doGrant = [&](int pid, int rid, int amt){
        int idx = ridToIdx[rid];
        resources[idx].available -= amt;
        procs[pid].hold[rid] += amt;
    };
    auto doFree = [&](int pid, int rid, int amt){
        if (!ridToIdx.count(rid)) return;
        int have = 0;
        auto it = procs[pid].hold.find(rid);
        if (it != procs[pid].hold.end()) have = it->second;
        int give = min(have, amt);
        if (give == 0) return;
        it->second -= give;
        if (it->second == 0) procs[pid].hold.erase(it);
        resources[ridToIdx[rid]].available += give;
    };

    auto pushReady = [&](int pid){
        procs[pid].state = READY_;
        int p = procs[pid].pr;
        if (p < 0) p = 0;
        if (p > 20) p = 20;
        rq[p].push_back(pid);
    };

    auto tryUnblockAll = [&](){
        vector<int> keep;
        for (int pid : blkList)
        {
            int rid = procs[pid].waitRid;
            int amt = procs[pid].waitAmt;
            if (canGrant(rid, amt))
            {
                doGrant(pid, rid, amt);
                procs[pid].waitRid = -1;
                procs[pid].waitAmt = 0;
                pushReady(pid);
            }
            else keep.push_back(pid);
        }
        blkList.swap(keep);
    };

    // init
    for (int i = 0; i < (int)procs.size(); i++)
    {
        procs[i].state = NEW_;
        procs[i].burstInd = 0;
        procs[i].opInd = 0;
        procs[i].remC = 0;
        procs[i].qleft = 0;
        procs[i].ioRem = 0;
        procs[i].finishTime = -1;
        procs[i].waitingTime = 0;
        procs[i].readyWait = 0;
        procs[i].starved = 0;
        procs[i].hold.clear();
        procs[i].waitRid = -1;
        procs[i].waitAmt = 0;
    }

    vector<GanttSeg> gantt;
    int lastPid = -2, segStart = 0;

    int cur = -1; // index in procs
    int t = 0;

    auto closeSegIfNeeded = [&](int runningPid){
        if (runningPid != lastPid)
        {
            if (lastPid != -2)
            {
                gantt.push_back({lastPid, segStart, t});
            }
            segStart = t;
            lastPid = runningPid;
        }
    };

    while (true)
    {
        // arrivals
        for (int i = 0; i < (int)procs.size(); i++)
            if (procs[i].state == NEW_ && procs[i].arrival == t)
                pushReady(i);

        // IO tick
        {
            vector<int> newIO;
            for (int pid : ioList)
            {
                procs[pid].ioRem--;
                if (procs[pid].ioRem == 0)
                {
                    procs[pid].burstInd++;   // finished IO burst
                    procs[pid].opInd = 0;
                    procs[pid].remC = 0;
                    pushReady(pid);
                }
                else newIO.push_back(pid);
            }
            ioList.swap(newIO);
        }

        // unblock attempt (in case resources were freed earlier tick)
        tryUnblockAll();

        // ---- READY aging ONCE per tick (no double-touch) ----
        vector<int> allReady;
        for (int pr = 0; pr <= 20; pr++)
        {
            while (!rq[pr].empty())
            {
                allReady.push_back(rq[pr].front());
                rq[pr].pop_front();
            }
        }

        for (int pid : allReady)
        {
            procs[pid].waitingTime++;
            procs[pid].readyWait++;

            if (procs[pid].readyWait >= 10)
            {
                procs[pid].readyWait = 0;
                if (procs[pid].pr > 0)
                {
                    procs[pid].pr--;
                    if (!procs[pid].starved)
                    {
                        procs[pid].starved = 1;
                        cout << "[Time " << t << "] STARVATION: P" << procs[pid].pid
                             << " waited >=10, AGING -> pr=" << procs[pid].pr << el;
                    }
                    else
                    {
                        cout << "[Time " << t << "] AGING: P" << procs[pid].pid
                             << " -> pr=" << procs[pid].pr << el;
                    }
                }
            }
            pushReady(pid);
        }
        // ----------------------------------------------------

        // best ready
        int bestPr = -1;
        for (int pr = 0; pr <= 20; pr++)
            if (!rq[pr].empty()) { bestPr = pr; break; }

        // preempt if someone higher pr exists
        if (cur != -1 && bestPr != -1 && bestPr < procs[cur].pr)
        {
            pushReady(cur);
            cur = -1;
        }

        // dispatch
        if (cur == -1 && bestPr != -1)
        {
            cur = rq[bestPr].front();
            rq[bestPr].pop_front();
            procs[cur].state = RUN_;
            procs[cur].qleft = Q;
            procs[cur].readyWait = 0;
        }

        int runningPid = (cur == -1 ? -1 : procs[cur].pid);
        closeSegIfNeeded(runningPid);

        // ---- RUN one tick ----
        if (cur != -1)
        {
            // helper: execute all instant ops (R/F) at current opInd
            auto execInstantOps = [&]()->bool{
                while (true)
                {
                    if (procs[cur].burstInd >= (int)procs[cur].bursts.size())
                        return true; // done

                    Burst &B = procs[cur].bursts[procs[cur].burstInd];
                    if (!B.isCPU) return true; // next is IO handled elsewhere

                    if (procs[cur].opInd >= (int)B.ops.size())
                    {
                        // finished CPU burst, move to next burst
                        procs[cur].burstInd++;
                        procs[cur].opInd = 0;
                        procs[cur].remC = 0;
                        continue;
                    }

                    Operation &op = B.ops[procs[cur].opInd];
                    if (op.kind == 'C') return true; // stop at CPU time op

                    if (op.kind == 'F')
                    {
                        doFree(cur, op.a, op.b);
                        procs[cur].opInd++;
                        // after releasing resources, try unblock
                        tryUnblockAll();
                        continue;
                    }

                    if (op.kind == 'R')
                    {
                        if (canGrant(op.a, op.b))
                        {
                            doGrant(cur, op.a, op.b);
                            procs[cur].opInd++;
                            continue;
                        }
                        // cannot grant -> BLOCK
                        procs[cur].state = BLK_;
                        procs[cur].waitRid = op.a;
                        procs[cur].waitAmt = op.b;
                        blkList.push_back(cur);
                        cur = -1;
                        return false; // stopped running
                    }

                    // unknown kind: skip
                    procs[cur].opInd++;
                }
            };

            // first, run all instant ops before CPU time
            if (cur != -1) execInstantOps();

            if (cur != -1)
            {
                // now we must be at a CPU 'C' op
                Burst &B = procs[cur].bursts[procs[cur].burstInd];
                Operation &op = B.ops[procs[cur].opInd]; // must be 'C'

                if (procs[cur].remC == 0) procs[cur].remC = op.a;

                procs[cur].remC--;
                procs[cur].qleft--;

                if (procs[cur].remC == 0)
                {
                    procs[cur].opInd++; // move to next op
                    // immediately execute following instant ops (R/F) at same time boundary
                    if (cur != -1) execInstantOps();
                }

                // if process finished bursts
                if (cur != -1 && procs[cur].burstInd >= (int)procs[cur].bursts.size())
                {
                    procs[cur].state = DONE_;
                    procs[cur].finishTime = t + 1;
                    cur = -1;
                }
                // if next burst is IO, move to IO
                else if (cur != -1 && procs[cur].burstInd < (int)procs[cur].bursts.size()
                         && !procs[cur].bursts[procs[cur].burstInd].isCPU)
                {
                    procs[cur].state = IO_;
                    procs[cur].ioRem = procs[cur].bursts[procs[cur].burstInd].ioDuration;
                    ioList.push_back(cur);
                    cur = -1;
                }
                // quantum expired
                else if (cur != -1 && procs[cur].qleft == 0)
                {
                    pushReady(cur);
                    cur = -1;
                }
            }
        }

        // termination
        bool done = true;
        for (int i = 0; i < (int)procs.size(); i++)
            if (procs[i].state != DONE_) { done = false; break; }

        if (done)
        {
            gantt.push_back({lastPid, segStart, t + 1});
            break;
        }

        t++;
        if (t > 200000) break;
    }

    // ---- PRINT GANTT as "start end pid" ----
    cout << "Gantt:" << el;
    for (auto &g : gantt)
    {
        if (g.pid == -1) cout << g.st << " " << g.en << " IDLE" << el;
        else cout << g.st << " " << g.en << " P" << g.pid << el;
    }

    double avgW = 0, avgT = 0;
    int cnt = 0;
    for (int i = 0; i < (int)procs.size(); i++)
    {
        if (procs[i].finishTime != -1)
        {
            ll turnaround = procs[i].finishTime - procs[i].arrival;
            ll waiting = procs[i].waitingTime;
            avgW += waiting;
            avgT += turnaround;
            cnt++;
            cout << "P" << procs[i].pid << " finish=" << procs[i].finishTime
                 << " turnaround=" << turnaround
                 << " waiting=" << waiting << el;
        }
    }
    if (cnt) { avgW /= cnt; avgT /= cnt; }
    cout << "Average waiting = " << avgW << el;
    cout << "Average turnaround = " << avgT << el;
}


int main()
{
    ios::sync_with_stdio(0);
    cin.tie(0);

    int t = 1;
    cin >> t;
    while (t--)
    {
        solve();
    }
    return 0;
}
