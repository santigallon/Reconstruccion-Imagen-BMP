#include <QCoreApplication>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QDebug>

// --------------------------
// Carga de archivo .txt con offset y sumas
// --------------------------

void loadTraceFile(const QString& filePath, int*& sums, int& offset, int& count) {
    QFile file(filePath);
    offset = 0;
    sums = nullptr;
    count = 0;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in >> offset;

        if (offset < 0) {
            qDebug() << "Advertencia: Offset negativo en archivo de rastreo";
            offset = 0;
        }

        // Contar líneas de sumas
        int tempCount = 0;
        in.readLine();  // Saltar primera línea (offset)
        while (!in.atEnd()) {
            in.readLine();
            tempCount++;
        }

        count = tempCount;
        sums = new int[count * 3];

        // Reiniciar lectura
        file.seek(0);
        in >> offset;
        in.readLine();  // Saltar primera línea

        for (int i = 0; i < count * 3; i += 3) {
            in >> sums[i] >> sums[i + 1] >> sums[i + 2];
        }

        file.close();
    } else {
        qDebug() << "Error al abrir archivo de rastreo:" << filePath;
    }
}

// --------------------------
// Operación inversa de enmascaramiento
// --------------------------

void reverseMasking(QImage& img, const QImage& mask, int* sums, int offset, int count) {
    int totalBytes = img.height() * img.bytesPerLine();
    int start = (offset % totalBytes + totalBytes) % totalBytes;
    uchar* imgData = img.bits();
    const uchar* maskData = mask.constBits();

    int imgBytesPerLine = img.bytesPerLine();
    int maskBytesPerLine = mask.bytesPerLine();

    for (int k = 0; k < count; ++k) {
        for (int c = 0; c < 3; ++c) {
            int pos = (start + k * 3 + c) % totalBytes;

            int imgY = pos / imgBytesPerLine;
            int imgX = (pos % imgBytesPerLine) / 3;
            if (imgX >= img.width() || imgY >= img.height()) continue;

            int imgIndex = imgY * imgBytesPerLine + imgX * 3 + c;

            // Máscara
            int maskPos = (k * 3 + c) % (mask.height() * mask.bytesPerLine());
            int maskY = maskPos / maskBytesPerLine;
            int maskX = (maskPos % mask.bytesPerLine()) / 3;
            if (maskX >= mask.width() || maskY >= mask.height()) continue;

            int maskIndex = maskY * maskBytesPerLine + maskX * 3 + c;

            imgData[imgIndex] = (sums[k * 3 + c] - maskData[maskIndex] + 256) % 256;
        }
    }
}

// --------------------------
// XOR entre dos imágenes (RGB)
// --------------------------

void applyXOR(QImage& img, const QImage& other) {
    if (img.size() != other.size() || img.format() != other.format()) {
        qDebug() << "Error: Imágenes incompatibles para XOR";
        return;
    }

    uchar* imgData = img.bits();
    const uchar* otherData = other.constBits();
    const int bytes = img.height() * img.bytesPerLine(); // incluye padding

    for (int i = 0; i < bytes; ++i) {
        imgData[i] ^= otherData[i];
    }
}

// --------------------------
// Deshacer rotación de 3 bits a la derecha
// --------------------------

void undoRotation(QImage& img) {
    int totalBytes = img.height() * img.bytesPerLine();
    uchar* imgData = img.bits();

    for (int i = 0; i < totalBytes; ++i) {
        imgData[i] = (imgData[i] << 3) | (imgData[i] >> 5); // Rotación a la izquierda
    }
}

// --------------------------
// Reconstrucción total de la imagen original
// --------------------------

QImage reconstructImage(const QImage& finalImg, const QImage& IM, const QImage& mask, int* sums1, int offset1, int count1, int* sums2, int offset2, int count2) {
    QImage workingImg = finalImg.copy();

    // Deshacer transformaciones en orden inverso
    reverseMasking(workingImg, mask, sums2, offset2, count2);
    applyXOR(workingImg, IM);
    undoRotation(workingImg);
    reverseMasking(workingImg, mask, sums1, offset1, count1);
    applyXOR(workingImg, IM);

    return workingImg;
}

// --------------------------
// Función principal
// --------------------------

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QImage I_O("I_O.bmp");
    QImage I_M("I_M.bmp");
    QImage mask("M.bmp");

    qDebug() << "Tamaños de las imágenes:";
    qDebug() << "- I_O:" << I_O.size();
    qDebug() << "- I_M:" << I_M.size();
    qDebug() << "- M:" << mask.size();

    qDebug() << "Estado de carga:";
    qDebug() << "- I_O.bmp:" << !I_O.isNull();
    qDebug() << "- I_M.bmp:" << !I_M.isNull();
    qDebug() << "- M.bmp:" << !mask.isNull();

    if (I_O.isNull() || I_M.isNull() || mask.isNull()) {
        qDebug() << "Error: No se pudieron cargar todas las imágenes.";
        return -1;
    }

    // Cargar archivos de rastreo
    int* sums1 = nullptr;
    int offset1 = 0;
    int count1 = 0;
    loadTraceFile("M1.txt", sums1, offset1, count1);

    int* sums2 = nullptr;
    int offset2 = 0;
    int count2 = 0;
    loadTraceFile("M2.txt", sums2, offset2, count2);

    QImage reconstructed = reconstructImage(I_O, I_M, mask, sums1, offset1, count1, sums2, offset2, count2);

    if (!reconstructed.isNull()) {
        reconstructed.save("original.bmp", "BMP");
        qDebug() << "Reconstrucción exitosa.";
    } else {
        qDebug() << "Error: La secuencia de transformaciones no coincidió.";
    }

    // Liberar memoria
    delete[] sums1;
    delete[] sums2;

    return 0;
}
