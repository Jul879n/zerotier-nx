.PHONY: run server qr
run:
	c++ -std=c++20 -Wall -Wextra -I../source ../source/core.cpp core_test.cpp -o core_test && ./core_test

# Servidor HTTP con shim de libnx (sockets POSIX); requiere curl.
server:
	c++ -std=c++20 -Wall -Wextra -Ishim -I../source ../source/http_server.cpp ../source/core.cpp server_test.cpp -o server_test && ./server_test

# Genera el QR con el mismo trazado que la GUI y lo decodifica con CoreImage (solo macOS).
qr:
	cc -c ../source/qrcodegen.c -I../source -o qrcodegen.o
	c++ -I../source qr_dump.cpp qrcodegen.o -o qr_dump
	./qr_dump 'http://192.168.100.100:8080/?t=ABC234' > qr.pgm
	python3 -I -c "from PIL import Image; Image.open('qr.pgm').save('qr.png')"
	swiftc -O qr_decode.swift -o qr_decode
	./qr_decode qr.png
