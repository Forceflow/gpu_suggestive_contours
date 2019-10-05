################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/BaseDrawer.cpp \
../src/Drawer.cpp \
../src/EdgeContourDrawer.cpp \
../src/FPSCounter.cpp \
../src/FaceContourDrawer.cpp \
../src/LineDrawer.cpp \
../src/Model.cpp \
../src/SuggestiveContourDrawer.cpp \
../src/Viewer.cpp 

CC_SRCS += \
../src/mesh_info.cc \
../src/vertex_info.cc 

OBJS += \
./src/BaseDrawer.o \
./src/Drawer.o \
./src/EdgeContourDrawer.o \
./src/FPSCounter.o \
./src/FaceContourDrawer.o \
./src/LineDrawer.o \
./src/Model.o \
./src/SuggestiveContourDrawer.o \
./src/Viewer.o \
./src/mesh_info.o \
./src/vertex_info.o 

CC_DEPS += \
./src/mesh_info.d \
./src/vertex_info.d 

CPP_DEPS += \
./src/BaseDrawer.d \
./src/Drawer.d \
./src/EdgeContourDrawer.d \
./src/FPSCounter.d \
./src/FaceContourDrawer.d \
./src/LineDrawer.d \
./src/Model.d \
./src/SuggestiveContourDrawer.d \
./src/Viewer.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -fopenmp -I"../include/trimesh2" -I"../include/trimesh2/GL" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -fopenmp -I"../include/trimesh2" -I"../include/trimesh2/GL" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


