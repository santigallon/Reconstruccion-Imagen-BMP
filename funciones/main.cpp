#include <iostream>
#include <QCoreApplication>


//estructura para imagenes BMP
struct BMPImage{
    int width, height;
    unsigned char* data;
    
    BMPImage()(int w, int h) : width(w), height(h) {
        data = new unsigned char[w * h * 3] //memoria dinamica 
    }
    
    ~BMPImage() {
        delete[] data; //liberar memoria 
    }
};

// archivos de rastreo
struct TraceFile {
    int offset ;
    int* sums; // arreglo dinamico 
    int count;
} ;

// operacion XOR
void apllyXOR (BMPImage& ing, const BMPImage& other){
    for (int i = 0 ; i < img.width * img.height * 3 ; ++i ){
        img.data[i] ^= other.data[i];
    }
};

// rotacion Right de bits
void rotateBitsRight(BMPImage& img, int bits){
    bits %= 8 ; //asegura que este entre 1 - 8 bits
    for (int i = 0 ; i < img.width * img.width * 3 ; ++i ){
        img.data[i] = (img.data[i] >> bits) | (img.data[i] << (8 - bits)); 
    }
};

// Rotación izquierda de bits
void rotateBitsLeft(BMPImage& img, int bits) {
    bits %= 8;
    for (int i = 0; i < img.width * img.height * 3; ++i) {
        img.data[i] = (img.data[i] << bits) | (img.data[i] >> (8 - bits));
    }
};

int  main (int argc, char *argv[]){
    QCoreAplication a(argc, argv);
    return a.exec();
}
