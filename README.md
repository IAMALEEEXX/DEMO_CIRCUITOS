# Analizador de Circuitos de Corriente Continua (CC) - SW303

Programa en lenguaje C (estándar C11) que calcula voltajes nodales, corrientes de rama y potencias mediante Análisis Nodal Modificado (MNA) y Eliminación Gaussiana con pivoteo parcial.

---

##  ¿Tu compañero necesita instalar algo?

- **NO necesita instalar absolutamente nada** si solo va a ejecutar y probar el programa con `circuito1.txt`, `cr01.txt`, `cr02.txt` o cualquier otro archivo `.txt` que él cree o modifique.
  - Al compilar en C con GCC, toda la lógica matemática y las librerías estándar (`stdio.h`, `stdlib.h`, `math.h`) quedan empaquetadas dentro del archivo `analizador.exe`.
  - Como los circuitos `.txt` se leen dinámicamente en tiempo de ejecución, se pueden cambiar valores de resistencias, fuentes o nodos en los archivos de texto y probarlos inmediatamente con `.\analizador.exe`.
- **Únicamente SÍ necesitaría instalar GCC** si abre `analizador.c`, realiza modificaciones al código fuente en C y desea compilar un nuevo archivo `.exe` en su computadora.

---

## 1. Ejecución directa en Windows (Sin instalar nada)

La carpeta ya incluye el ejecutable compilado (`analizador.exe`).

1. Abre esta carpeta en **VS Code** o en cualquier terminal de Windows (`PowerShell` o `CMD`).
2. *(Opcional)* En consolas antiguas, puedes activar la codificación UTF-8:
   ```powershell
   chcp 65001
   ```
3. Ejecuta cualquiera de los casos de prueba incluidos:
   ```powershell
   .\analizador.exe circuito1.txt
   .\analizador.exe cr01.txt
   .\analizador.exe cr02.txt
   ```
4. Al finalizar cada análisis, los resultados se muestran en pantalla y se exportan automáticamente al archivo `resultados.csv`.

---

## 2. Cómo compilar desde el código fuente (Solo si modificas analizador.c)

Si realizas cambios en el archivo `analizador.c`, necesitas tener instalado el compilador GCC (MinGW-w64) para generar un nuevo `.exe`:

### Paso 1: Instalar GCC en Windows (solo una vez)
Abre PowerShell y ejecuta:
```powershell
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
```
*(O si tienes Chocolatey: `choco install mingw -y` en PowerShell como Administrador).*

Reinicia la terminal o VS Code después de instalarlo.

### Paso 2: Compilar el código
Ejecuta en la terminal dentro de esta carpeta:
```powershell
gcc -Wall -Wextra -std=c11 analizador.c -o analizador.exe
```

---

## 3. Formato de los archivos de circuito (.txt)

Puedes crear nuevos circuitos o editar los existentes respetando el formato de una línea por componente:

- **Resistencia:**  
  `R <nombre> <nodo_A> <nodo_B> <valor_ohmios>`  
  *Ejemplo:* `R R1 1 2 100`

- **Fuente de voltaje:**  
  `V <nombre> <nodo_+> <nodo_-> <valor_voltios>`  
  *Ejemplo:* `V V1 1 0 10`

- **Nodo de referencia (Tierra):**  
  Siempre debe ser el nodo `0`.

- **Comentarios:**  
  Líneas que inician con `#` son ignoradas por el analizador.

---

## 4. Archivos del proyecto

Para compartir el proyecto (por ejemplo, en un archivo `.zip`), asegúrate de incluir:
- `analizador.c`: Código fuente del analizador en C.
- `analizador.exe`: Ejecutable listo para usar en Windows (64-bit).
- `circuito1.txt`, `cr01.txt`, `cr02.txt`: Casos de prueba incluidos.
- `README.md`: Documentación e instrucciones del proyecto.

