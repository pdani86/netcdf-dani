#ifndef NETCDF_DANI_BITPARTITION_H
#define NETCDF_DANI_BITPARTITION_H

#include <iostream>

#include "NcFile.h"

inline void transform_to_bitpartitioned_raw(NcFile& ncFile) {
    auto dimlen = ncFile.dims();
    int elevation_var_id = ncFile.getVarIdByName("elevation");
    std::cout << "dimlen: " << dimlen[0] << " * " << dimlen[1] << std::endl;
    size_t width = dimlen[0];
    size_t count[2] = {1, width};
    std::cout << "Elevation var id: " << elevation_var_id << std::endl;
    auto rowBuf = std::vector<int16_t>(width);
    auto rowBufBitPartitioned = std::vector<int16_t>(width, 0);
//    std::ofstream ofs("D:/out_full_16_bit_partitioned.raw", std::ios::binary);


    // --------- CHECK

    std::size_t srcPopCount = 0; // for checking correctness
    std::size_t dstPopCount = 0;

    auto popCount = [](const auto& num) -> std::size_t {
        std::size_t sum = 0;
        for(int bitIx = 0; bitIx < sizeof(num)*8; ++bitIx) {
            sum += 1 * (0 != (num & (1 << bitIx)));
        }
        return sum;
    };
//    std::cout << "popCount(259)=" << popCount(259) << std::endl;
//    if(popCount(259) != 3) {
//        throw std::runtime_error("popcount check failed");
//    }
    static_assert(popCount(259) == 3);
    auto popCountVec = [&popCount](const auto& v) -> std::size_t {
        std::size_t sum = 0;
        for(const auto& elem : v) {
            sum += popCount(elem);
        }
        return sum;
    };

    // --------- /CHECK

    for(int rowIx = 0; rowIx < dimlen[1]; ++rowIx) {
        size_t start[2] = {(size_t)rowIx, 0};
        std::fill(rowBufBitPartitioned.begin(), rowBufBitPartitioned.end(), 0);
        ncFile.getInt64Data(rowBuf.data(), elevation_var_id, start, count);
        for(int colIx = 0; colIx < dimlen[0]; ++colIx) {
            for(int srcBitPos=0; srcBitPos < 16; ++srcBitPos) {
                bool bitOn = (rowBuf[colIx] & (1 << srcBitPos)) != 0;
                auto dstElemIx = srcBitPos * (width/16) + (colIx/16);
                rowBufBitPartitioned[dstElemIx] |= (1 << (colIx % 16)) * bitOn;
            }
        }

        if(popCountVec(rowBuf) != popCountVec(rowBufBitPartitioned)) {
            /*std::ofstream ofs_src("D:/popcnt_dbg_src.raw", std::ios::binary);
            std::ofstream ofs_dst("D:/popcnt_dbg_dst.raw", std::ios::binary);
            ofs_src.write((const char*)rowBuf.data(), 2);
            for(int i=0;i<16;++i) {
                ofs_dst.write((const char *) &rowBufBitPartitioned.data()[i*5400], 2);
            }*/

//            ofs_dst.write((const char*)rowBufBitPartitioned.data(), rowBufBitPartitioned.size()*2);
            throw std::runtime_error("popcount check failed");
        }

//        ofs.write((const char*)rowBufBitPartitioned.data(), rowBufBitPartitioned.size()*sizeof(decltype(rowBufBitPartitioned)::value_type));
    }
//    ofs.flush();
}

#endif //NETCDF_DANI_BITPARTITION_H
