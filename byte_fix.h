#pragma once

// 1. Desactiva std::byte si no lo necesitas
#define _HAS_STD_BYTE 0

// 2. Configuraciones esenciales de Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT

// 3. Incluye Windows.h con protección
#include <windows.h>

// 4. Limpia posibles conflictos
#ifdef small
#undef small  // Windows a veces define 'small' como macro
#endif