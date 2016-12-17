################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../deteccionDeadlock.o \
../dibujarNivel.o \
../nivel-test.o \
../planificacion.o \
../solicitudes.o 

C_SRCS += \
../deteccionDeadlock.c \
../dibujarNivel.c \
../nivel-test.c \
../planificacion.c \
../solicitudes.c 

OBJS += \
./deteccionDeadlock.o \
./dibujarNivel.o \
./nivel-test.o \
./planificacion.o \
./solicitudes.o 

C_DEPS += \
./deteccionDeadlock.d \
./dibujarNivel.d \
./nivel-test.d \
./planificacion.d \
./solicitudes.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2016-2c-Stranger-Code/biblioteca-charmander" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


