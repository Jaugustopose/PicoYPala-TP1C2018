################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../comunicacion/comunicacion.c 

OBJS += \
./comunicacion/comunicacion.o 

C_DEPS += \
./comunicacion/comunicacion.d 


# Each subdirectory must supply rules for building sources it contributes
comunicacion/%.o: ../comunicacion/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-1c-Pico-y-Pala/commons" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


