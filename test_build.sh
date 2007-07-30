(./configure --enable-debug --enable-db --enable-pcapmmap&& make clean && make -j 16) || exit 1
(./configure --enable-debug --enable-db && make clean && make -j 16) || exit 1
(./configure --enable-db && make clean && make -j 16) || exit 1
(./configure --enable-debug && make clean && make -j 16) || exit 1
(./configure --enable-pcapmmap&& make clean && make -j 16) || exit 1
(./configure && make clean && make -j 16) || exit 1
