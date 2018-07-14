################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/conexiones.c \
../src/planificacion.c \
../src/planificador.c 

OBJS += \
./src/conexiones.o \
./src/planificacion.o \
./src/planificador.o 

C_DEPS += \
./src/conexiones.d \
./src/planificacion.d \
./src/planificador.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-1c-Pico-y-Pala/comunicacion" -I"/home/utnso/workspace/tp-2018-1c-Pico-y-Pala/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


