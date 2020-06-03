
cd ../build

rm -r cmk
mkdir cmk
cd cmk

cmake ../../
make -j8

make install

