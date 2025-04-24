# Reconstructor de Imágenes BMP

Programa en C++/Qt para revertir transformaciones aplicadas a una imagen BMP mediante operaciones de bits y enmascaramiento.

## Requisitos
- Qt 5 o superior
- Compilador C++11
- Archivos de entrada: Io.bmp, IM.bmp, M.bmp, M1.txt, M2.txt

## Compilación y Ejecución
```bash
# Clonar el repositorio
git clone https://github.com/tu-usuario/bmp-reconstructor.git
cd bmp-reconstructor

# Compilar con Qt
qmake BMPProcessor.pro
make

# Ejecutar
./BMPProcessor
