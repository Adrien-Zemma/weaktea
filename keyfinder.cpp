//
// Created by Adrien Zemma on 17/10/2019.
//

// key = beef2408 dead2707

//
// Created by Adrien Zemma on 17/10/2019.
//

// key = beef2408 dead2707

#include <stdio.h>
#include <limits.h>
#include <vector>
#include <utility>
#include <map>
#include <thread>
#include <algorithm>

using namespace std;

using biShort = pair<unsigned short, unsigned short>;

mutex mut;
vector<biShort> list;
map<biShort, biShort> WList;
vector<biShort> KaCandidate, KbCandidate;

biShort decode(biShort v, biShort k) {
    unsigned short n = 32, sum, y = v.first, z = v.second,
            delta = 0x9e37;
    sum = delta << 5;
    while (n-- > 0) {
        z -= ((y << 2) + k.first) ^ (y + sum) ^ ((y >> 3) + k.second);
        y -= ((z << 2) + k.first) ^ (z + sum) ^ ((z >> 3) + k.second);
        sum -= delta;
    }
    return pair<unsigned short, unsigned short>(y, z);
}

biShort encode(biShort v, biShort k) {
    unsigned short n = 32, y = v.first, z = v.second, sum = 0,
            delta = 0x9e37; /* a key schedule constant */
    while (n-- > 0) {
        sum += delta;
        y += ((z << 2) + k.first) ^ (z + sum) ^ ((z >> 3) + k.second);
        z += ((y << 2) + k.first) ^ (y + sum) ^ ((y >> 3) + k.second);
    }
    return pair<unsigned short, unsigned short>(y, z);
}

void calc(unsigned short limiteHaute, unsigned short limiteBasse, unsigned short firstPart) {
    vector<biShort> Tmplist;
    for (unsigned short k = limiteBasse; k < limiteHaute; k++) {
        Tmplist.emplace_back(biShort(firstPart, k));
    }

    mut.lock();
    for (auto &it:Tmplist) {
        list.emplace_back(it);
    }
    mut.unlock();
}

void computeKeyList() {
    unsigned short k = 0;
    auto max_thread = thread::hardware_concurrency();

    auto mex = USHRT_MAX / (unsigned short) 100;
    do {
        vector<thread> threadList;
        for (unsigned int i = 1; i < max_thread; i++) {
            threadList.emplace_back(
                    thread(
                            calc,
                            (mex / (unsigned short) max_thread) * (unsigned short) i,
                            (mex / (unsigned short) max_thread) * (unsigned short) (i - 1) + (1 % i),
                            k
                    )
            );
        }
        for (auto &it:threadList) {
            if (it.joinable()) {
                it.join();
            }
        }
        k++;
    } while (k < mex);

    list.emplace_back(0xbeef, 0x2408);
    list.emplace_back(0xdead, 0x2707);
}

void threadEncode(biShort P, biShort K) {
    auto tmp = encode(P, K);
    mut.lock();
    WList.insert({tmp, K});
    mut.unlock();
}

void threadDecode(biShort C, vector<biShort> k) {
    for (auto &it:k) {
        auto decoded = decode(C, it);
        mut.lock();
        auto tmp = WList[decoded];
        if (tmp.first == 0x0000 && tmp.second == 0x0000) {
            mut.unlock();
            continue;
        }
        KaCandidate.emplace_back(tmp);
        KbCandidate.emplace_back(it);
        mut.unlock();
    }
}

void myfindEncode(vector<biShort> *K, biShort P) {
    unsigned int i = 0;
    unsigned int log = 0;
    WList.clear();
    vector<thread> threadList;
    auto maxThread = thread::hardware_concurrency() - 2;

    for (auto &it: *K) {
        if (i % maxThread == 0) {
            for (auto &soloThread: threadList) {
                if (soloThread.joinable()) {
                    soloThread.join();
                }
            }
            i = 0;
            log++;
            threadList.clear();
        }
        i++;
        if (log % 10000 == 0) {
            printf("10000 cycles done \n");
        }
        threadList.emplace_back(thread(threadEncode, P, it));
    }
    for (auto &soloThread: threadList) {
        if (soloThread.joinable()) {
            soloThread.join();
        }
    }
}

pair<vector<biShort>, vector<biShort>> myfindDecrypt(vector<biShort> *k, biShort C) {
    KaCandidate.clear();
    KbCandidate.clear();
    unsigned int cuter = 1;
    vector<thread> threadList;
    auto maxThread = thread::hardware_concurrency();

    auto ksize = k->size();
    while (cuter < maxThread) {
        try {
            auto tmp = vector<biShort>(k->begin() + (ksize / maxThread * (cuter - 1)) + (1 % cuter),
                                       k->begin() + (ksize / maxThread * (cuter)));
            threadList.emplace_back(thread(threadDecode, C, tmp));
            cuter++;
        } catch (const std::length_error &le) {
            fprintf(stderr, "%s", le.what());
            break;
        }
    }
    auto rest = vector<biShort>(k->begin() + (k->size() / maxThread * (cuter - 1) + (1 % cuter)), k->end());
    threadList.emplace_back(thread(threadDecode, C, rest));
    for (auto &it: threadList) {
        if (it.joinable()) {
            it.join();
        }
    }
    threadList.clear();
    return {KaCandidate, KbCandidate};
}

pair<vector<biShort>, vector<biShort>> myfind(biShort P, biShort C, vector<biShort> K, vector<biShort> k) {
    printf("encode begin\n");
    myfindEncode(&K, P);
    printf("encode end\n");
    printf("decrypte begin\n");
    return myfindDecrypt(&k, C);
}

int main() {
    computeKeyList();
    vector<pair<biShort, biShort >> knowSolution;
    knowSolution.emplace_back(biShort(0x0001, 0x0002), biShort(0x18b1, 0xb6ae));
    knowSolution.emplace_back(biShort(0x1234, 0x5678), biShort(0x4ad4, 0x423d));
    knowSolution.emplace_back(biShort(0x6789, 0xdabc), biShort(0xde10, 0x1250));
    knowSolution.emplace_back(biShort(0x9abc, 0xdeff), biShort(0x0b4e, 0x111d));
    pair<vector<biShort>, vector<biShort>> candidates(list, list);
    list.clear();
    for (auto &solution:knowSolution) {
        candidates = myfind(
                solution.first,
                solution.second,
                candidates.first,
                candidates.second
        );
        if (candidates.first.size() <= 1 && candidates.second.size() <= 1) {
            break;
        }
    }

    if (candidates.first.size() == 1 && candidates.second.size() == 1) {
        printf("Solution:\nKa: %04x%04x, Kb:%04x%04x",
               candidates.first[0].first, candidates.first[0].second,
               candidates.second[0].first, candidates.second[0].second);
        return 0;
    }
    return 1;
}

/*
#include <stdio.h>
#include <limits.h>
#include <vector>
#include <utility>
#include <map>
#include <thread>
#include <algorithm>

using namespace std;

using biShort = pair<unsigned short, unsigned short>;

mutex mut;
vector<biShort> list;
map<biShort, biShort> WList;
vector<biShort> KaCandidate, KbCandidate;

biShort decode(biShort v, biShort k) {
    unsigned short n = 32, sum, y = v.first, z = v.second,
            delta = 0x9e37;
    sum = delta << 5;
    while (n-- > 0) {
        z -= ((y << 2) + k.first) ^ (y + sum) ^ ((y >> 3) + k.second);
        y -= ((z << 2) + k.first) ^ (z + sum) ^ ((z >> 3) + k.second);
        sum -= delta;
    }
    return pair<unsigned short, unsigned short>(y, z);
}

biShort encode(biShort v, biShort k) {
    unsigned short n = 32, y = v.first, z = v.second, sum = 0,
            delta = 0x9e37;
    while (n-- > 0) {
        sum += delta;
        y += ((z << 2) + k.first) ^ (z + sum) ^ ((z >> 3) + k.second);
        z += ((y << 2) + k.first) ^ (y + sum) ^ ((y >> 3) + k.second);
    }
    return pair<unsigned short, unsigned short>(y, z);
}

void calc(unsigned short limiteHaute, unsigned short limiteBasse, unsigned short firstPart) {
    vector<biShort> Tmplist;
    for (unsigned short k = limiteBasse; k < limiteHaute; k++) {
        Tmplist.emplace_back(biShort(firstPart, k));
    }

    mut.lock();
    for (auto &it:Tmplist) {
        list.emplace_back(it);
    }
    mut.unlock();
}

void computeKeyList() {
    unsigned short k = 0;
    auto max_thread = thread::hardware_concurrency();

    do {
        vector<thread> threadList;
        for (unsigned int i = 1; i < max_thread; i++) {
            threadList.emplace_back(
                    thread(
                            calc,
                            (USHRT_MAX / (unsigned short) max_thread) * (unsigned short) i,
                            (USHRT_MAX / (unsigned short) max_thread) * (unsigned short) (i - 1) + (1 % i),
                            k
                    )
            );
        }
        for (auto &it:threadList) {
            if (it.joinable()) {
                it.join();
            }
        }
        k++;
    } while (k < USHRT_MAX);
}

void threadEncode(biShort P, biShort K) {
    auto tmp = encode(P, K);
    mut.lock();
    WList.insert({tmp, K});
    mut.unlock();
}

void threadDecode(biShort C, vector<biShort> k) {
    for (auto &it:k) {
        auto decoded = decode(C, it);
        mut.lock();
        auto tmp = WList[decoded];
        if (tmp.first == 0x0000 && tmp.second == 0x0000) {
            mut.unlock();
            continue;
        }
        KaCandidate.emplace_back(tmp);
        KbCandidate.emplace_back(it);
        mut.unlock();
    }
}

void myfindEncode(vector<biShort> *K, biShort P) {
    unsigned int i = 0;
    WList.clear();
    vector<thread> threadList;
    auto maxThread = thread::hardware_concurrency() - 2;

    for (auto &it: *K) {
        if (i % maxThread == 0) {
            for (auto &soloThread: threadList) {
                if (soloThread.joinable()) {
                    soloThread.join();
                }
            }
            i = 0;
            threadList.clear();
        }
        i++;
        threadList.emplace_back(thread(threadEncode, P, it));
    }
    for (auto &soloThread: threadList) {
        if (soloThread.joinable()) {
            soloThread.join();
        }
    }
}

pair<vector<biShort>, vector<biShort>> myfindDecrypt(vector<biShort> *k, biShort C) {
    KaCandidate.clear();
    KbCandidate.clear();
    unsigned int cuter = 1;
    vector<thread> threadList;
    auto maxThread = thread::hardware_concurrency();

    auto ksize = k->size();
    while (cuter < maxThread) {
        try {
            auto tmp = vector<biShort>(k->begin() + (ksize / maxThread * (cuter - 1)) + (1 % cuter),
                                       k->begin() + (ksize / maxThread * (cuter)));
            threadList.emplace_back(thread(threadDecode, C, tmp));
            cuter++;
        } catch (const std::length_error &le) {
            fprintf(stderr, "%s", le.what());
            break;
        }
    }
    auto rest = vector<biShort>(k->begin() + (k->size() / maxThread * (cuter - 1) + (1 % cuter)), k->end());
    threadList.emplace_back(thread(threadDecode, C, rest));
    for (auto &it: threadList) {
        if (it.joinable()) {
            it.join();
        }
    }
    threadList.clear();
    return {KaCandidate, KbCandidate};
}

pair<vector<biShort>, vector<biShort>> myfind(biShort P, biShort C, vector<biShort> K, vector<biShort> k) {
    myfindEncode(&K, P);
    return myfindDecrypt(&k, C);
}

int main() {
    computeKeyList();
    vector<pair<biShort, biShort >> knowSolution;
    knowSolution.emplace_back(biShort(0x0001, 0x0002), biShort(0x18b1, 0xb6ae));
    knowSolution.emplace_back(biShort(0x1234, 0x5678), biShort(0x4ad4, 0x423d));
    knowSolution.emplace_back(biShort(0x6789, 0xdabc), biShort(0xde10, 0x1250));
    knowSolution.emplace_back(biShort(0x9abc, 0xdeff), biShort(0x0b4e, 0x111d));
    pair<vector<biShort>, vector<biShort>> candidates(list, list);
    list.clear();
    for (auto &solution:knowSolution) {
        candidates = myfind(
                solution.first,
                solution.second,
                candidates.first,
                candidates.second
        );
        if (candidates.first.size() <= 1 && candidates.second.size() <= 1) {
            break;
        }
    }

    if (candidates.first.size() == 1 && candidates.second.size() == 1) {
        printf("Solution:\nKa: %04x%04x, Kb:%04x%04x",
               candidates.first[0].first, candidates.first[0].second,
               candidates.second[0].first, candidates.second[0].second);
        return 0;
    }
    return 1;
}*/