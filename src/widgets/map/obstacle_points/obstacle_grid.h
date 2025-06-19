#pragma once

#include <QList>
#include <QVector>

class ObstacleGridMap {
public:
    ObstacleGridMap(int rows, int cols)
        : rows(rows), cols(cols), data(rows, QVector<uint8_t>(cols, 0)) {}

    void updateRow(int row, const QList<uint8_t>& values) {
        if(row < 0 || row >= rows || values.size() != cols) return;
        for(int c = 0; c < cols; ++c) {
            data[row][c] = values[c];
        }
    }

    int rowCount() const { return rows; }
    int colCount() const { return cols; }
    uint8_t value(int r, int c) const { return data[r][c]; }

private:
    int rows, cols;
    QVector<QVector<uint8_t>> data;
};