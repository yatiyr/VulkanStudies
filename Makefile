CFLAGS = -std=c++17 -g3
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
STB_INCLUDE_PATH = include

VulkanTest: main.cpp
	g++ $(CFLAGS) -o build/VulkanTest main.cpp $(LDFLAGS) -I$(STB_INCLUDE_PATH)

.PHONY: test clean

test: VulkanTest
	./build/VulkanTest

clean:
	rm -f VulkanTest