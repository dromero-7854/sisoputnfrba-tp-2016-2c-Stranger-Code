################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Conexion/conexion.c 

OBJS += \
./src/Conexion/conexion.o 

C_DEPS += \
./src/Conexion/conexion.d 


# Each subdirectory must supply rules for building sources it contributes
src/Conexion/%.o: ../src/Conexion/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


