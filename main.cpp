#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>


int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // Cargar recursos
    BMPImage Io = loadBMP("Io.bmp");
    BMPImage IM = loadBMP("IM.bmp");
    BMPImage mask = loadBMP("M.bmp");

    // Cargar archivos de rastreo
    TraceFile traces[2];
    traces[0] = loadTraceFile("M1.txt");
    traces[1] = loadTraceFile("M2.txt");

    // Reconstruir imagen
    BMPImage reconstructed = reconstructImage(Io, IM, mask, traces, 2);

    // Validar resultados
    bool isValid = true;
    for (int i = 0; i < 2; i++) {
        if (!validateStep(reconstructed, traces[i], mask)) {
            isValid = false;
            break;
        }
    }

    if (isValid) {
        saveBMP("original.bmp", reconstructed);
        qDebug() << "Reconstrucción exitosa!";
    } else {
        qDebug() << "Error: La secuencia de operaciones no coincide";
    }

    // Liberar memoria
    for (int i = 0; i < 2; i++) {
        delete[] traces[i].sums;
    }

    return 0;
}


// Estructura para imágenes BMP
struct BMPImage {
    int width, height;
    unsigned char* data;

    BMPImage(int w, int h) : width(w), height(h) {
        data = new unsigned char[w * h * 3]; // Memoria dinámica
    }

    ~BMPImage() {
        delete[] data; // Liberar memoria
    }
};

// Estructura para archivos de rastreo
struct TraceFile {
    int offset;
    int* sums; // Arreglo dinámico
    int count;
};

// Operaciones de bits

// Operación XOR
void applyXOR(BMPImage& img, const BMPImage& other) {
    for (int i = 0; i < img.width * img.height * 3; ++i) {
        img.data[i] ^= other.data[i];
    }
}

// Rotación derecha de bits
void rotateBitsRight(BMPImage& img, int bits) {
    bits %= 8; // Asegurar 1-8 bits
    for (int i = 0; i < img.width * img.height * 3; ++i) {
        img.data[i] = (img.data[i] >> bits) | (img.data[i] << (8 - bits));
    }
}

// Rotación izquierda de bits
void rotateBitsLeft(BMPImage& img, int bits) {
    bits %= 8;
    for (int i = 0; i < img.width * img.height * 3; ++i) {
        img.data[i] = (img.data[i] << bits) | (img.data[i] >> (8 - bits));
    }
}

// Funciones

// Función para cargar una imagen BMP (24 bits sin compresión)
BMPImage loadBMP(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error al abrir el archivo BMP:" << filePath;
        return BMPImage(0, 0);
    }

    // Leer cabeceras (asumiendo cabecera estándar de 54 bytes)
    char header[54];
    file.read(header, 54);

    // Extraer ancho y alto desde la cabecera
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];

    BMPImage img(width, height);
    file.read(reinterpret_cast<char*>(img.data), width * height * 3);

    file.close();
    return img;
}

// Función para guardar una imagen BMP (24 bits sin compresión)
void saveBMP(const QString& filePath, const BMPImage& img) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Error al guardar el archivo BMP:" << filePath;
        return;
    }

    // Crear cabecera BMP básica (54 bytes)
    char header[54] = {
        'B', 'M',                     // Tipo de archivo
        0, 0, 0, 0,                   // Tamaño total (se actualiza después)
        54, 0, 0, 0,                   // Offset de datos
        40, 0, 0, 0,                   // Tamaño de la cabecera de información
        img.width & 0xFF, (img.width >> 8) & 0xFF, (img.width >> 16) & 0xFF, (img.width >> 24) & 0xFF, // Ancho
        img.height & 0xFF, (img.height >> 8) & 0xFF, (img.height >> 16) & 0xFF, (img.height >> 24) & 0xFF, // Alto
        1, 0,                          // Planos (siempre 1)
        24, 0,                         // Bits por píxel (24)
        0, 0, 0, 0,                    // Compresión (0 = sin compresión)
        0, 0, 0, 0,                    // Tamaño de los datos de imagen
        0, 0, 0, 0,                    // Resolución horizontal
        0, 0, 0, 0,                    // Resolución vertical
        0, 0, 0, 0,                    // Colores en la paleta
        0, 0, 0, 0                     // Colores importantes
    };

    // Calcular tamaño total del archivo
    int dataSize = img.width * img.height * 3;
    *(int*)&header[2] = 54 + dataSize; // Tamaño total

    file.write(header, 54);
    file.write(reinterpret_cast<const char*>(img.data), dataSize);
    file.close();
}

// Función para leer archivos de rastreo (offset y sumas)
TraceFile loadTraceFile(const QString& filePath) {
    QFile file(filePath);
    TraceFile tf;
    tf.offset = 0;
    tf.sums = nullptr;
    tf.count = 0;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error al abrir el archivo de rastreo:" << filePath;
        return tf;
    }

    QTextStream in(&file);
    in >> tf.offset; // Leer offset

    // Contar líneas de sumas
    int lineCount = 0;
    while (!in.atEnd()) {
        in.readLine();
        lineCount++;
    }

    // Reiniciar lectura para obtener las sumas
    file.seek(0);
    in >> tf.offset; // Saltar offset nuevamente

    // Reservar memoria para las sumas
    tf.count = lineCount;
    tf.sums = new int[lineCount * 3];

    // Leer sumas RGB
    for (int i = 0; i < lineCount; i++) {
        int r, g, b;
        in >> r >> g >> b;
        tf.sums[i*3] = r;
        tf.sums[i*3+1] = g;
        tf.sums[i*3+2] = b;
    }

    file.close();
    return tf;
}

// Revertir el enmascaramiento usando la máscara y el archivo de rastreo
void reverseMasking(BMPImage& img, const BMPImage& mask, const TraceFile& tf) {
    int start = tf.offset % (img.width * img.height * 3);
    for (int k = 0; k < tf.count * 3; k++) {
        int imgPos = (start + k) % (img.width * img.height * 3);
        int maskPos = k % (mask.width * mask.height * 3);
        img.data[imgPos] = (tf.sums[k] - mask.data[maskPos] + 256) % 256; // Manejo de desbordamiento
    }
}

// Reconstruir la imagen aplicando operaciones inversas en orden
BMPImage reconstructImage(const BMPImage& finalImg, const BMPImage& IM, const BMPImage& mask, TraceFile* traces, int numTraces) {
    BMPImage currentImg = finalImg;

    // Procesar en orden inverso (última transformación primero)
    for (int i = numTraces - 1; i >= 0; i--) {
        // 1. Revertir enmascaramiento
        reverseMasking(currentImg, mask, traces[i]);

        // 2. Probar combinaciones de operaciones inversas
        // Ejemplo: XOR seguido de rotación izquierda (inversa de rotación derecha)
        applyXOR(currentImg, IM);
        rotateBitsLeft(currentImg, 3);
    }

    return currentImg;
}

// Validar si una porción de la imagen coincide con el archivo de rastreo
bool validateStep(const BMPImage& img, const TraceFile& tf, const BMPImage& mask) {
    int start = tf.offset % (img.width * img.height * 3);
    int matches = 0;

    for (int k = 0; k < tf.count * 3; k++) {
        int imgPos = (start + k) % (img.width * img.height * 3);
        int maskPos = k % (mask.width * mask.height * 3);
        int expected = (img.data[imgPos] + mask.data[maskPos]) % 256;

        if (expected == tf.sums[k]) matches++;
    }

    return (matches >= tf.count * 3 * 0.95); // 95% de coincidencia (tolerancia)
}
