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

using namespace std;

using biShort = pair<unsigned short, unsigned short>;

mutex mut;
vector<biShort> list;
map<biShort, biShort> WList;

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

    do {
        vector<thread> threadList;
        for (unsigned int i = 1; i < max_thread; i++) {
            threadList.emplace_back(
                    thread(
                            calc,
                            (USHRT_MAX / (unsigned short) max_thread) * (unsigned short) i,
                            (USHRT_MAX / (unsigned short) max_thread) * (unsigned short) (i - 1),
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

void threadEncode(biShort P, biShort k) {
    auto tmp = encode(P, k);
    mut.lock();
    WList.insert({tmp, k});
    mut.unlock();
}

void threadDecode(biShort C, biShort k, vector<biShort> *KaCandidate, vector<biShort> *KbCandidate) {
    auto decoded = decode(C, k);
    mut.lock();
    if (WList[decoded].first == 0x0000 && WList[decoded].second == 0x0000) {
        mut.unlock();
        return;
    }
    KaCandidate->emplace_back(WList[decoded]);
    KbCandidate->emplace_back(k);
    mut.unlock();
}

pair<vector<biShort>, vector<biShort>> myfind(biShort P, biShort C, vector<biShort> *K, vector<biShort> *k) {
    WList.clear();
    vector<biShort> KaCandidate, KbCandidate;

    printf("encode begin\n");
    vector<thread> threadList;
    auto maxThread = thread::hardware_concurrency();
    int i = 0;
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
    threadList.clear();
    printf("encode ending\n");

    printf("decode begin\n");
    i = 0;
    for (auto &it: *k) {
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
        threadList.emplace_back(thread(threadDecode, C, it, &KaCandidate, &KbCandidate));
    }

    for (auto &soloThread: threadList) {
        if (soloThread.joinable()) {
            soloThread.join();
        }
    }

    printf("decode ending\n");
    return {KaCandidate, KbCandidate};
}

int main() {
    computeKeyList();
    printf("Key Gen: %ld\n", list.size());

    vector<pair<biShort, biShort >> knowSolution;

    knowSolution.emplace_back(biShort(0x0001, 0x0002), biShort(0x18b1, 0xb6ae));
    knowSolution.emplace_back(biShort(0x1234, 0x5678), biShort(0x4ad4, 0x423d));
    knowSolution.emplace_back(biShort(0x6789, 0xdabc), biShort(0xde10, 0x1250));
    knowSolution.emplace_back(biShort(0x9abc, 0xdeff), biShort(0x0b4e, 0x111d));
    printf("knowSolution added\n");

    pair<vector<biShort>, vector<biShort>> candidates(list, list);
    list.clear();

    int i = 0;
    for (auto &solution:knowSolution) {
        printf("itération: %d\n", i++);
        printf("size: %ld, %ld\n", candidates.first.size(), candidates.second.size());
        candidates = myfind(
                solution.first,
                solution.second,
                &candidates.first,
                &candidates.second
        );
        printf("candidate size: %ld\n", candidates.first.size());
    }

    if (candidates.first.size() == 1 && candidates.second.size() == 1) {
        printf("Solution:\nKa: %04x%04x, Kb:%04x%04x",
               candidates.first[0].first, candidates.first[0].second,
               candidates.second[0].first, candidates.second[0].second);
        return 0;
    }
    printf("Possible answers\nKa:\n");
    for (auto &it:candidates.first) {
        printf("|%04x%04x\n", it.first, it.second);
    }
    printf("\nKb:\n");
    for (auto &it:candidates.second) {
        printf("/ %04x%04x\n", it.first, it.second);
    }
    return 1;
}