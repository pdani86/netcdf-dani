
#include "netcdf.h"
#include <iostream>
#include <vector>
#include <fstream>

#include <optional>


void handle_error(int status) {
    std::cout << "error " << status << std::endl;
}


int print_info(int ncid) {
    int status = 0;

    // Get NetCDF file format version
    int format;
    status = nc_inq_format(ncid, &format);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error getting NetCDF format version.\n");
        return 1;
    }
    printf("NetCDF format version: %s\n", (format == NC_FORMAT_CLASSIC) ? "Classic" : "Enhanced");

    // Get the number of dimensions in the file
    int ndims;
    status = nc_inq_ndims(ncid, &ndims);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error getting the number of dimensions.\n");
        return 1;
    }
    printf("Number of dimensions: %d\n", ndims);

    // Get dimension information
    for (int dimid = 0; dimid < ndims; dimid++) {
        char dimname[NC_MAX_NAME + 1];
        size_t dimlen;
        status = nc_inq_dim(ncid, dimid, dimname, &dimlen);
        if (status != NC_NOERR) {
            fprintf(stderr, "Error getting dimension information.\n");
            return 1;
        }
        printf("Dimension %d: Name='%s', Length=%lu\n", dimid, dimname, (unsigned long)dimlen);
    }

    // Get the number of variables in the file
    int nvars;
    status = nc_inq_nvars(ncid, &nvars);
    if (status != NC_NOERR) {
        fprintf(stderr, "Error getting the number of variables.\n");
        return 1;
    }
    printf("Number of variables: %d\n", nvars);

    // Get variable information
    for (int varid = 0; varid < nvars; varid++) {
        char varname[NC_MAX_NAME + 1];
        nc_type xtype;
        int ndims;
        int dimids[NC_MAX_VAR_DIMS];
        status = nc_inq_var(ncid, varid, varname, &xtype, &ndims, dimids, NULL);
        if (status != NC_NOERR) {
            fprintf(stderr, "Error getting variable information.\n");
            return 1;
        }
        printf("Variable %d: Name='%s', Type=%d, NumDims=%d\n", varid, varname, xtype, ndims);
    }
    return 0;
}

int read_data_2d(int ncid) {
    int status = 0;
    char dimname[NC_MAX_NAME + 1];
    size_t dimlen[2] = {0,0};
    status = nc_inq_dim(ncid, 0, dimname, &dimlen[0]);
    status = nc_inq_dim(ncid, 1, dimname, &dimlen[1]);
    std::cout << "dimlen: " << dimlen[0] << " * " << dimlen[1] << std::endl;

    size_t start[2];
    start[0] = 0;
    start[1] = 0;

    size_t width = 1; //dimlen[0];
    size_t height = dimlen[1];

    size_t count[2];
    count[0] = height;
    count[1] = width;

    int scale = 40;

    int elevation_var_id = 0;
    status = nc_inq_varid(ncid, "elevation", &elevation_var_id);
    std::cout << "Elevation var id: " << elevation_var_id << std::endl;
    if (status != NC_NOERR) {
        fprintf(stderr, "Error getting variable ID for 'elevation'.\n");
        return 1;
    }

    auto rowBuf = std::make_unique<int16_t[]>(width * height);

    std::vector<uint8_t> rawImageData;
    size_t scaledDownSize[] = {dimlen[0] / scale, dimlen[1] / scale};
    rawImageData.resize((scaledDownSize[0]) * (scaledDownSize[1]));

    for(int rowIx = 0; rowIx < dimlen[1]; rowIx += scale) {
        if(0 == rowIx % 100) {
            std::cout << "row " << rowIx << std::endl;
        }
        start[0] = 0;
        start[1] = rowIx;
        status = nc_get_vara_short(ncid, elevation_var_id, start, count, rowBuf.get());
        if(status != NC_NOERR) {
            handle_error(status);
            return status;
        }

        for(int colIx = 0; colIx < dimlen[0] /*- scale*/; colIx += scale) {
            int sum = 0;
            sum = rowBuf[colIx];
            /*for(int i=0;i<scale;++i) {
                sum += rowBuf[colIx + i];
            }*/
            rawImageData[(rowIx/scale) * scaledDownSize[0] + (colIx/scale)] = (((sum /* / scale*/)) / 256) + 128;
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


class NcFile {
public:
    ~NcFile();
    NcFile(const NcFile&) = delete;
    NcFile& operator=(const NcFile&) = delete;
    NcFile(NcFile&&) noexcept = default;
    NcFile& operator=(NcFile&&) noexcept = default;

    static NcFile openForRead(const char* filename);

    int nativeHandle() { return _ncid.value(); }
private:
    explicit NcFile(int ncid) : _ncid(ncid) {}
private:
    std::optional<int> _ncid{};
};

NcFile NcFile::openForRead(const char* filename) {
    int ncid = 0;
    int status = nc_open(filename, NC_NOWRITE, &ncid);
    if(status != NC_NOERR) {
        throw std::runtime_error("Open error");
    }
    return NcFile(ncid);
}

NcFile::~NcFile() {
    if(_ncid) {nc_close(*_ncid);}
}


/*
 *
 */

int main() {
    //const char* nc_datafile = "D:\\other\\data\\geo\\gebco_2023\\GEBCO_2023.nc";
    const char* nc_datafile = "C:\\dani\\other\\GEBCO_2023.nc";
    auto nc_file = NcFile::openForRead(nc_datafile);
    print_info(nc_file.nativeHandle());
    read_data_2d(nc_file.nativeHandle());
    return 0;
}
