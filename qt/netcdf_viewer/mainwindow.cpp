#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <QImage>
#include <cmath>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(&_scene);

//    GPS hunGps1{48.64698758285766, 14.999676527732783};
//    GPS hunGps2{45.50885749858355, 23.417102803214963};
//    GPS hunGps1{44.6, 15.5};

    createOverviewData();

    connect(ui->graphicsView, SIGNAL(mouse_clicked(int, int)), this, SLOT(on_graphicsview_mouse_clicked(int,int)));

    GPS hunGps1{47.162, 19.503};
    ui->latitudeSlider->setValue(hunGps1.lat()*1000);
    ui->longitudeSlider->setValue(hunGps1.lon()*1000);

    update();
}

void MainWindow::on_graphicsview_mouse_clicked(int x, int y) {
//    std::cout << "mouse click pos: " << x << ", " << y << std::endl;
    if(_areaMode) {
        return;
    }
    auto xx = static_cast<double>(x);
    auto yy = static_cast<double>(y);
//    auto pixWidth = 1.0/_overviewWidth;
    auto lon = ((xx/_overviewWidth) * 360.0) - 180.0;
    auto lat = 90.0 - ((yy/_overviewHeight) * 180.0);
    ui->longitudeSlider->setValue(lon * 1000);
    ui->latitudeSlider->setValue(lat * 1000);
    update();
}

void MainWindow::createOverviewData() {
    std::ifstream ifs(_overviewFilename, std::ios::binary);
    if(!ifs.is_open()) {
        std::cout << "couldn't open file" << std::endl;
        return;
    }

    _overviewData.resize(_overviewWidth * _overviewHeight);
    ifs.read((char*)_overviewData.data(), _overviewData.size() * sizeof(int16_t));
}

uint8_t MainWindow::heightToGray(int16_t height, int16_t min, int16_t max) {
    double t = static_cast<double>(height - min) / (max - min);
    t = std::clamp(t, 0.0, 1.0);
    auto value = static_cast<uint8_t>(0.5 + t * 255);
    return value;
}

QColor MainWindow::heightToColor(int16_t height, int16_t min, int16_t max) {
    //double blueLimit = _blueLimit;
    double blueLimit = 0.0;
    double greenFieldLimit = _greenLimit;
    double brownFieldLimit = _brownLimit;
    if(height < blueLimit) {
        double t = (height - min) / (blueLimit - (double)min);
        auto val = std::clamp(static_cast<int>(t*255), 0, 255);
        return QColor(0,0, val);
    }
    if(height < greenFieldLimit) {
        double t = (height - blueLimit) / (greenFieldLimit - blueLimit);
        t = 0.5 + 0.5 * t;
        auto val = std::clamp(static_cast<int>(t*255), 0, 255);
        return QColor(0, val, 0);
    }
    if( height < brownFieldLimit) {
        double t = (height - greenFieldLimit) / (brownFieldLimit - greenFieldLimit);
        t = 0.2 + 0.5 * t;
        auto val = std::clamp(static_cast<int>(t*255), 0, 255);
        return QColor(val, val/4, val/4);
    }

    double t = (height - brownFieldLimit) / (max - brownFieldLimit);
    t = 0.5 + 0.5 * t;
    auto val = std::clamp(static_cast<int>(t*255), 0, 255);
    return QColor(val, val, val);
}

QImage MainWindow::createAreaImageGray() {
    GPS gpsCenter{ui->latitudeSlider->value() / 1000.0, ui->longitudeSlider->value() / 1000.0};
    const int w = _areaImageWidth;
    const int h = _areaImageHeight;

    QImage img(w, h, QImage::Format_Grayscale8);
    uint8_t* row = img.bits();
    size_t bytesPerLine = img.bytesPerLine();

    int16_t height_min = ui->heightMin->value();
    int16_t height_max = ui->heightMax->value();

    auto& ncFile = getNcFile();
    auto varId = ncFile.getVarIdByName(_elevationVarName.c_str());

    auto dataCols = ncFile.dims().at(0);
    auto dataRows = ncFile.dims().at(1);

    double stepPerDegree = dataCols / 360.0;
    GpsToOffsetConverter converter(stepPerDegree, dataRows/2, dataCols/2);

    auto offsetCenter = converter.convert(gpsCenter);
    auto offset = offsetCenter; // south-west corner
    offset.latLon[0] += h/2;
    offset.latLon[1] -= w/2;

    size_t offsets[] = {offset.latLon[0], offset.latLon[1]};
    size_t counts[] = {1, (size_t)w};
    std::vector<int16_t> rowBuf;
    rowBuf.resize(w);

    for(int y = 0; y < h; ++y, row += bytesPerLine, --offsets[0]) {
        ncFile.getInt64Data(rowBuf.data(), varId, offsets, counts);
        for(int x = 0; x < w; ++x) {
            int16_t srcVal = rowBuf[x];
            auto value = heightToGray(srcVal, height_min, height_max);
//            if(isInArea) {value /= 2;}
            row[x] = value;
        }
    }
    return img;
}

QImage MainWindow::createOverviewImageGray() {
    QImage img(_overviewWidth, _overviewHeight, QImage::Format_Grayscale8);
    uint8_t* row = img.bits();
    size_t bytesPerLine = img.bytesPerLine();

    size_t srcRowSize = _overviewWidth;
    const int16_t* srcRow = _overviewData.data() + srcRowSize * (_overviewHeight - 1);

    int16_t height_min = ui->heightMin->value();
    int16_t height_max = ui->heightMax->value();

    for(int y = 0; y < _overviewHeight; ++y, row += bytesPerLine, srcRow -= srcRowSize) {
        for(int x = 0; x < _overviewWidth; ++x) {
            auto value = heightToGray(srcRow[x], height_min, height_max);
            row[x] = value;
        }
    }
    return img;
}

QImage MainWindow::createAreaImageColor() {
    GPS gpsCenter{ui->latitudeSlider->value() / 1000.0, ui->longitudeSlider->value() / 1000.0};
    const int w = _areaImageWidth;
    const int h = _areaImageHeight;

    QImage img(w, h, QImage::Format_RGB888);
    uint8_t* row = img.bits();
    size_t bytesPerLine = img.bytesPerLine();

    int16_t height_min = ui->heightMin->value();
    int16_t height_max = ui->heightMax->value();

    auto& ncFile = getNcFile();
    auto varId = ncFile.getVarIdByName(_elevationVarName.c_str());

    auto dataCols = ncFile.dims().at(0);
    auto dataRows = ncFile.dims().at(1);

    double stepPerDegree = dataCols / 360.0;
    GpsToOffsetConverter converter(stepPerDegree, dataRows/2, dataCols/2);

    auto offsetCenter = converter.convert(gpsCenter);
    auto offset = offsetCenter; // south-west corner
    offset.latLon[0] += h/2;
    offset.latLon[1] -= w/2;

    size_t offsets[] = {offset.latLon[0], offset.latLon[1]};
    size_t counts[] = {1, (size_t)w};
    std::vector<int16_t> rowBuf;
    rowBuf.resize(w);

    for(int y = 0; y < h; ++y, row += bytesPerLine, --offsets[0]) {
        ncFile.getInt64Data(rowBuf.data(), varId, offsets, counts);
        for(int x = 0; x < w; ++x) {
            int16_t srcVal = rowBuf[x];
            auto value = heightToColor(srcVal, height_min, height_max);
            row[3*x] = value.red();
            row[3*x+1] = value.green();
            row[3*x+2] = value.blue();
        }
    }
    return img;
}

QImage MainWindow::createOverviewImageColor() {
    QImage img(_overviewWidth, _overviewHeight, QImage::Format_RGB888);
    uint8_t* row = img.bits();
    size_t bytesPerLine = img.bytesPerLine();

    size_t srcRowSize = _overviewWidth;
    const int16_t* srcRow = _overviewData.data() + srcRowSize * (_overviewHeight - 1);

    int16_t height_min = ui->heightMin->value();
    int16_t height_max = ui->heightMax->value();

    for(int y = 0; y < _overviewHeight; ++y, row += bytesPerLine, srcRow -= srcRowSize) {
        for(int x = 0; x < _overviewWidth; ++x) {
            auto value = heightToColor(srcRow[x], height_min, height_max);
            row[3*x] = value.red();
            row[3*x+1] = value.green();
            row[3*x+2] = value.blue();
        }
    }
    return img;
}

QImage MainWindow::createAreaImage() {
    return ui->colorMap->isChecked() ? createAreaImageColor() : createAreaImageGray();
}

QImage MainWindow::createOverviewImage() {
    return ui->colorMap->isChecked() ? createOverviewImageColor() : createOverviewImageGray();
}


Offset2D MainWindow::getOverviewOffsetFromGps(const GPS& gps) const {
    const double stepPerDegree = _overviewWidth / 360.0;
    GpsToOffsetConverter converter(stepPerDegree, _overviewHeight/2, _overviewWidth/2);
    return converter.convert(gps);
}

void MainWindow::update() {
    GPS gpsCenter{ui->latitudeSlider->value() / 1000.0, ui->longitudeSlider->value() / 1000.0};
    ui->statusbar->showMessage(QString("%1 %2").arg(gpsCenter.lat()).arg(gpsCenter.lon()));
    if(_areaMode) {
        updateArea();
    } else {
        updateWorld();
    }
}

void MainWindow::updateWorld() {
    _scene.clear();

    auto img = createOverviewImage();

    _scene.addPixmap(QPixmap::fromImage(img));

    GPS gpsCenter{ui->latitudeSlider->value() / 1000.0, ui->longitudeSlider->value() / 1000.0};
    auto targetOffset = getOverviewOffsetFromGps(gpsCenter);
    double radius = 20.0;
    auto targetX = targetOffset.latLon[1];
    auto targetY = (_overviewHeight-1) - targetOffset.latLon[0];
    _scene.addEllipse(targetX-radius, targetY-radius, radius*2.0, radius*2.0, QPen(QColor(255,0,0)));
}

void MainWindow::updateArea() {
    _scene.clear();

    auto img = createAreaImage();

    _scene.addPixmap(QPixmap::fromImage(img));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_heightMin_sliderMoved(int position)
{
//    update();
}


void MainWindow::on_heightMin_sliderReleased()
{
    update();
}


void MainWindow::on_heightMax_sliderReleased()
{
    update();
}


void MainWindow::on_areaMode_toggled(bool checked)
{
    _areaMode = checked;
    update();
}


void MainWindow::on_longitudeSlider_sliderReleased()
{
    update();
}


void MainWindow::on_latitudeSlider_sliderReleased()
{
    update();
}


void MainWindow::on_longitudeSlider_valueChanged(int value)
{
    if(!_areaMode) {
        updateWorld();
    }
}


void MainWindow::on_latitudeSlider_valueChanged(int value)
{
    if(!_areaMode) {
        updateWorld();
    }
}


void MainWindow::on_colorMap_toggled(bool checked)
{
    update();
}


void MainWindow::on_greenLimit_valueChanged(int value)
{
    _greenLimit = value;
    update();
}


void MainWindow::on_brownLimit_valueChanged(int value)
{
    _brownLimit = value;
    update();
}

