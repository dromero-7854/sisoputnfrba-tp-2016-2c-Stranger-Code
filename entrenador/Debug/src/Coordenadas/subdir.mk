################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Coordenadas/coordenadas.c 

OBJS += \
./src/Coordenadas/coordenadas.o 

C_DEPS += \
./src/Coordenadas/coordenadas.d 


# Each subdirectory must supply rules for building sources it contributes
src/Coordenadas/%.o: ../src/Coordenadas/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


