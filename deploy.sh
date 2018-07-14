git clone https://github.com/sisoputnfrba/so-commons-library
git clone https://github.com/sisoputnfrba/parsi
git clone https://github.com/sisoputnfrba/Pruebas-ESI
cd so-commons-library
sudo make install
cd ..
cd parsi
sudo make install
cd ..
cd comunicacion/Debug
make all
cd ..
cd ..
cd coordinador/Debug
make all
cd ..
cd ..
cd planificador/Debug
make all
cd ..
cd ..
cd instancia/Debug
make all
cd ..
cd ..
cd esi/Debug
make all
cd ..
cd ..
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2018-1c-Pico-y-Pala/comunicacion/Debug