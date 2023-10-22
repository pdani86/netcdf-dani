#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <mygraphicsview.h>
#include <vector>

#include "NcFile.h"
#include "gps.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct AreaData {
    Offset2D southWestOffset;
    int width{};
    int height{};
    std::unique_ptr<int16_t> data;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void createOverviewData();
    void update();
    void updateArea();
    void updateWorld();

    uint8_t heightToGray(int16_t height, int16_t min = -12000, int16_t max = 9000);
    QColor heightToColor(int16_t height, int16_t min = -12000, int16_t max = 9000);
    QImage createAreaImage();
    QImage createOverviewImage();

    QImage createAreaImageGray();
    QImage createOverviewImageGray();
    QImage createAreaImageColor();
    QImage createOverviewImageColor();

    Offset2D getOverviewOffsetFromGps(const GPS& gps) const;

    NcFile& getNcFile() /*const*/ {
        if(!_ncFile.has_value()) {
            _ncFile = NcFile::openForRead(_ncFilename.c_str());
        }
        return _ncFile.value();
    }

private slots:
    void on_heightMin_sliderMoved(int position);

    void on_heightMin_sliderReleased();

    void on_heightMax_sliderReleased();

    void on_areaMode_toggled(bool checked);

    void on_longitudeSlider_sliderReleased();

    void on_latitudeSlider_sliderReleased();

    void on_longitudeSlider_valueChanged(int value);

    void on_latitudeSlider_valueChanged(int value);

    void on_colorMap_toggled(bool checked);

    void on_greenLimit_valueChanged(int value);

    void on_brownLimit_valueChanged(int value);

    void on_graphicsview_mouse_clicked(int x, int y);

private:
    Ui::MainWindow *ui;

    QGraphicsScene _scene;
    std::vector<int16_t> _overviewData;
    size_t _overviewWidth = 2160;
    size_t _overviewHeight = 1080;
    const std::string _overviewFilename = "C:\\dani\\github\\netcdf-dani\\out16_40_753_2021.raw";

    bool _areaMode = false;
    const int _areaImageWidth = 1920;
    const int _areaImageHeight = 1080;

    int _greenLimit = 2000;
    int _brownLimit = 4000;

    const std::string _ncFilename = "D:\\data\\geo\\gebco_2023\\GEBCO_2023.nc";
    const std::string _elevationVarName = "elevation";

    std::optional<NcFile> _ncFile;
};

#endif // MAINWINDOW_H
