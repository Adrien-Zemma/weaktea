//
// Created by Adrien Zemma on 17/10/2019.
//

// key = beef2408 dead2707

#include <stdio.h>
#include <limits.h>
#include <vector>
#include <utility>
#include <map>

using namespace std;

using biShort = pair<unsigned short, unsigned short>;

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

vector<biShort> computeKeyList() {
    vector<biShort> list;
    biShort k;
    do {
        k.second = 0;
        do {
            list.emplace_back(k);
            k.second++;
        } while (k.second < USHRT_MAX);
        k.first++;
    } while (k.first < USHRT_MAX);
    return list;
}

pair<vector<biShort>, vector<biShort>> myfind(biShort P, biShort C, vector<biShort> *K, vector<biShort> *k) {
    map<biShort, biShort> WList;
    vector<biShort> KaCandidate, KbCandidate;

    printf("encode begin\n");
    for (auto &it: *K) {
        WList.insert({encode(P, it), it});
    }
    printf("encode ending\n");
    printf("decode begin\n");
    for (auto &it: *k) {
        auto decoded = decode(C, it);
        if (WList[decoded].first == 0x0000 && WList[decoded].second == 0x0000) {
            continue;
        }
        KaCandidate.emplace_back(WList[decoded]);
        KbCandidate.emplace_back(it);
    }
    printf("decode ending\n");
    return {KaCandidate, KbCandidate};
}


int main() {
    auto KeyList = computeKeyList();
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);
    vector<pair<biShort, biShort >> knowSolution;

    printf("Key Gen\n");

    knowSolution.emplace_back(biShort(0x0001, 0x0002), biShort(0x18b1, 0xb6ae));
    knowSolution.emplace_back(biShort(0x1234, 0x5678), biShort(0x4ad4, 0x423d));
    knowSolution.emplace_back(biShort(0x6789, 0xdabc), biShort(0xde10, 0x1250));
    knowSolution.emplace_back(biShort(0x9abc, 0xdeff), biShort(0x0b4e, 0x111d));
    printf("knowSolution added\n");
    pair<vector<biShort>, vector<biShort>> candidates(KeyList, KeyList);
    int i = 0;
    for (auto &solution:knowSolution) {
        printf("iteration: %d\n", i++);
        if (candidates.first.size() != 1 && candidates.second.size() != 1) {
            candidates = myfind(
                    biShort(0x0001, 0x0002),
                    biShort(0x18b1, 0xb6ae),
                    &candidates.first,
                    &candidates.second
            );
            printf("candidate size: %ld\n", candidates.first.size());
        } else {
            break;
        }
    }

    if (candidates.first.size() == 1 && candidates.second.size() == 1) {
        printf("Solution:\nKa: %04x%04x, Kb:%04x%04x",
               candidates.first[0].first, candidates.first[0].second,
               candidates.second[0].first, candidates.second[0].second);
        return 0;
    }
    printf("Possible answers\nKa:");
    for (auto &it:candidates.first) {
        printf("%04x%04x\n", it.first, it.second);
    }
    printf("\nKb:");
    for (auto &it:candidates.second) {
        printf("%04x%04x\n", it.first, it.second);
    }
    return 1;
}