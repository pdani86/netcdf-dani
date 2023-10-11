
#include "netcdf.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <optional>
#include <string>

#include "NcFile.h"


void handle_error(int status) {
    std::cout << "error " << status << std::endl;
}


int print_info(const NcFile& ncFile) {
    printf("NetCDF format version: %s\n", (ncFile.fileFormat() == NC_FORMAT_CLASSIC) ? "Classic" : "Enhanced");
    printf("Number of dimensions: %d\n", (int)ncFile.nDims());

    // Get dimension information
    auto ndims = ncFile.nDims();
    for (int dimid = 0; dimid < ndims; dimid++) {
        auto dimlen = ncFile.dims()[dimid];
        auto dimname = ncFile.dimNames()[dimid];
        std::cout << "Dimension " << dimid << " Name='" << dimname << "', Length=" << std::to_string(dimlen) << "\n";
    }

    std::cout << "Variables:\n";
    auto nvars = ncFile.nVariables();
    for(int varIx = 0; varIx < nvars; ++varIx) {
        auto varInfo = ncFile.getVariableInfo(varIx);
        std::cout << "var[" << varInfo.ix << "]: " << varInfo.name << " - " << varInfo.type << " ";
        bool isFirst = true;
        for(const auto& dim : varInfo.dims) {
            if(!isFirst) std::cout << ",";
            isFirst = false;
            std::cout << dim;
        }
        std::cout << "\n";
    }
    std::cout.flush();

    return 0;
}

int transform_elevation_data(const NcFile& ncFile) {
    constexpr int scale = 40;

    auto dimlen = ncFile.dims();
    int elevation_var_id = ncFile.getVarIdByName("elevation");

    std::cout << "dimlen: " << dimlen[0] << " * " << dimlen[1] << std::endl;

    size_t width = dimlen[0];
//    size_t height = dimlen[1];

    size_t count[2];
    count[0] = 1;
    count[1] = width;


    std::cout << "Elevation var id: " << elevation_var_id << std::endl;
//    int nRowPerBlockRead = 1000;
    int nRowPerBlockRead = 1;
    auto rowBuf = std::make_unique<int16_t[]>(nRowPerBlockRead * width);

    std::vector<uint8_t> rawImageData;
    size_t scaledDownSize[] = {dimlen[0] / scale, dimlen[1] / scale};
    rawImageData.resize((scaledDownSize[0]) * (scaledDownSize[1]));

    for(int rowIx = 0; rowIx < dimlen[1]; rowIx += scale) {
        if(0 == rowIx % 100) {
            std::cout << "row " << rowIx << std::endl;
        }
        //size_t start[2] = {0, (size_t)rowIx};
        size_t start[2] = {(size_t)rowIx, 0};
        ncFile.getInt64Data(rowBuf.get(), elevation_var_id, start, count);

        for(int colIx = 0; colIx < dimlen[0]; colIx += scale) {
            std::int64_t sum = 0;
            sum = rowBuf[colIx];
            for(int i=0;i<scale;++i) {
                sum += rowBuf[colIx + i];
            }
            rawImageData[(rowIx/scale) * scaledDownSize[0] + (colIx/scale)] = (((sum / scale)) / 256) + 128;
        }
    }
    std::ofstream ofs("out.raw", std::ios::binary);
    ofs.write((const char*)rawImageData.data(), rawImageData.size());
    ofs.flush();

//    for(int colIx = 0; colIx < width; colIx += 1000) {
//        std::cout << "[" << colIx << "]: " << (int)rowBuf[colIx] << "\n";
//    }
    std::cout << std::endl;

    return 0;
}


int main() {
    //const char* nc_datafile = "D:\\other\\data\\geo\\gebco_2023\\GEBCO_2023.nc";
    const char* nc_datafile = "C:\\dani\\other\\GEBCO_2023.nc";
    auto nc_file = NcFile::openForRead(nc_datafile);
    print_info(nc_file);
    transform_elevation_data(nc_file);
    return 0;
}
