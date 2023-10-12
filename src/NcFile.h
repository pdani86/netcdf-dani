#ifndef NETCDF_DANI_NCFILE_H
#define NETCDF_DANI_NCFILE_H

#include <vector>
#include <optional>
#include <stdexcept>

#include "netcdf.h"

class NcHandle {
public:
    explicit NcHandle(int ncid) : _ncid(ncid) {}
    ~NcHandle();

    NcHandle(const NcHandle&) = delete;
    NcHandle& operator=(const NcHandle&) = delete;

    NcHandle(NcHandle&& other) noexcept {
        std::swap(this->_ncid, other._ncid);
    }

    NcHandle& operator=(NcHandle&& other) noexcept {
        if(this == &other) {return *this;}
        std::swap(this->_ncid, other._ncid);
        return *this;
    }

    int handle() const {
        return _ncid.value();
    }

    std::optional<int> _ncid{};
};

inline NcHandle::~NcHandle() {
    if(_ncid) {
        nc_close(*_ncid);
    }
}

class NcFile {
public:
    NcFile(const NcFile&) = delete;
    NcFile& operator=(const NcFile&) = delete;
    NcFile(NcFile&& other) noexcept = default;
    NcFile& operator=(NcFile&& other) noexcept = default;

    static NcFile openForRead(const char* filename);

    int fileFormat() const;

    int nativeHandle() { return _ncHandle._ncid.value(); }

    std::size_t nDims() const { return _dims.size(); }
    const std::vector<std::size_t>& dims() const { return _dims; }
    const std::vector<std::string>& dimNames() const { return _dimNames; }

    std::size_t nVariables() const { return _variableNames.size(); }

    struct VariableInfo {
        int ix{};
        std::string name;
        nc_type type{};
        std::vector<int> dims;
    };

    VariableInfo getVariableInfo(int varIx) const {
        VariableInfo info;
        info.ix = varIx;
        info.name = _variableNames.at(varIx);
        info.type = _variableTypes.at(varIx);
        info.dims = _variableDimIds.at(varIx);
        return info;
    }

    int getVarIdByName(const char* varName) const;
    void getInt64Data(int16_t* dst, int varId, std::size_t* offset, std::size_t* count) const;

private:
    explicit NcFile(int ncid) : _ncHandle(ncid) {}
    static void _throwOnError(int status) {if(status != NC_NOERR) throw std::runtime_error("nc error"); }
    void _initDimensions();
    void _initVariableInfo();

private:
    NcHandle _ncHandle;
    std::vector<std::size_t> _dims;
    std::vector<std::string> _dimNames;

    std::vector<std::string> _variableNames;
    std::vector<nc_type> _variableTypes;
    std::vector<std::vector<int>> _variableDimIds;
};

inline int NcFile::getVarIdByName(const char* varName) const {
    int varId = 0;
    _throwOnError(nc_inq_varid(*_ncHandle._ncid, varName, &varId));
    return varId;
}

inline void NcFile::getInt64Data(int16_t* dst, int varId, std::size_t* offset, std::size_t* count) const {
    _throwOnError(nc_get_vara_short(_ncHandle.handle(), varId, offset, count, dst));
}

inline void NcFile::_initDimensions() {
    int ndims = 0;
    _throwOnError(nc_inq_ndims(_ncHandle.handle(), &ndims));
    _dims.resize(ndims);
    _dimNames.resize(ndims);
    for(int dimIx = 0; dimIx < ndims; ++dimIx) {
        char dimname[NC_MAX_NAME + 1];
        size_t dimlen{};
        _throwOnError(nc_inq_dim(_ncHandle.handle(), dimIx, dimname, &dimlen));
        _dims[dimIx] = dimlen;
        _dimNames[dimIx] = std::string(dimname);
    }
}

inline void NcFile::_initVariableInfo() {
    int nvars = 0;
    _throwOnError(nc_inq_nvars(_ncHandle.handle(), &nvars));
    _variableNames.resize(nvars);
    _variableTypes.resize(nvars);
    _variableDimIds.resize(nvars);
    // Get variable information
    for(int varix = 0; varix < nvars; varix++) {
        char varname[NC_MAX_NAME + 1];
        nc_type xtype;
        int ndims = 0;
        int dimids[NC_MAX_VAR_DIMS];
        _throwOnError(nc_inq_var(_ncHandle.handle(), varix, varname, &xtype, &ndims, dimids, NULL));
        _variableNames[varix] = std::string(varname);
        _variableTypes[varix] = xtype;

        _variableDimIds[varix].resize(ndims);
        for(int dimIx = 0; dimIx < ndims; ++dimIx) {
            _variableDimIds[varix][dimIx] = dimids[dimIx];
        }
    }
}

inline int NcFile::fileFormat() const {
    int format = 0;
    _throwOnError(nc_inq_format(_ncHandle.handle(), &format));
    return format;
}

inline NcFile NcFile::openForRead(const char* filename) {
    int ncid = 0;
    int status = nc_open(filename, NC_NOWRITE, &ncid);
    if(status != NC_NOERR) {
        throw std::runtime_error("Open error");
    }
    auto file = NcFile(ncid);
    file._initDimensions();
    file._initVariableInfo();
    return file;
}


#endif //NETCDF_DANI_NCFILE_H
