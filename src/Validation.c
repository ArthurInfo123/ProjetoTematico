#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "validation.h"
#include "ui.h"

int validate_amount(double value) {
    if (value <= 0) {
        ui_error("Valor deve ser maior que zero.");
        return 0;
    }
    return 1;
}

int validate_date(const char *date) {
    // formato esperado: DD/MM/AAAA
    if (strlen(date) != 10) {
        ui_error("Data invalida. Use o formato DD/MM/AAAA.");
        return 0;
    }

    if (date[2] != '/' || date[5] != '/') {
        ui_error("Data invalida. Use o formato DD/MM/AAAA.");
        return 0;
    }

    // verificar que os outros caracteres sao digitos
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (date[i] < '0' || date[i] > '9') {
            ui_error("Data invalida. Use o formato DD/MM/AAAA.");
            return 0;
        }
    }

    int day   = atoi(date);
    int month = atoi(date + 3);
    int year  = atoi(date + 6);

    if (month < 1 || month > 12) {
        ui_error("Mes invalido. Use um valor entre 01 e 12.");
        return 0;
    }

    if (year < 1900 || year > 2100) {
        ui_error("Ano invalido.");
        return 0;
    }

    // dias por mes (ano nao bissexto)
    int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    // ano bissexto
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        days_in_month[1] = 29;

    if (day < 1 || day > days_in_month[month - 1]) {
        ui_error("Dia invalido para o mes informado.");
        return 0;
    }

    return 1;
}