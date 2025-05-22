# Gestor de Biblioteca

Este es nuestro proyecto de semestre para desarrollar un sistema de gestión de biblioteca utilizando tecnologías modernas.

## 🚀 Características principales

- 📚 **Gestión de libros**: Añadir libros
- 🔍 **Búsqueda avanzada**: Encontrar libros por título, autor o por ambos
- 🔄 **Préstamos**: Registrar préstamos y devoluciones de libros
- 🔒 **Autenticación**: Sistema de login seguro para diferentes tipos de usuarios

## 🛠 Tecnologías utilizadas

| Tecnología | Descripción |
|------------|-------------|
| C++17 | Lenguaje de programación principal |
| PostgreSQL | Sistema de gestión de base de datos |
| libpq-fe | Biblioteca C++ para PostgreSQL |
| Git | Control de versiones |
| GitHub | Plataforma de colaboración |

<div align="left">
  <img src="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/cplusplus/cplusplus-original.svg" height="40" alt="cplusplus logo"  />
  <img width="12" />
  <img src="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/postgresql/postgresql-original.svg" height="40" alt="postgresql logo"  />
  <img width="12" />
  <img src="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/vscode/vscode-original.svg" height="40" alt="vscode logo"  />
  <img src="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/git/git-original.svg" height="40" alt="git logo"  />
  <img width="12" />
   <img src="https://skillicons.dev/icons?i=github" height="40" alt="github logo"  />
</div>

###


## 👥 Equipo de desarrollo

- **Antony Castañón** 
- **Osman Hernández** 
- **Darwin Hernández** 

## Instalación

Para usar esta aplicación, sigue los siguientes pasos:

1. Clona el repositorio en tu máquina local:
    ```bash
    https://github.com/cstantony1996/Gestor_De_Biblioteca.git
    ```
2. Asegúrate de tener **PostgreSQL** instalado y configurado.
3. Configura la base de datos en el archivo de configuración (si es necesario).
4. Compila y ejecuta el proyecto.

## 🗄️ Configuración de la base de datos

Para que el sistema funcione correctamente, necesitas crear las tablas necesarias en PostgreSQL. Ya hemos preparado un archivo llamado `DATABASE.sql` que contiene toda la estructura de la base de datos.

### ✅ Pasos para configurarlo:

1. Asegúrate de tener **PostgreSQL** instalado en tu equipo.

2. Abre tu herramienta de administración de base de datos favorita (como pgAdmin o psql desde la terminal).

3. Crea una base de datos vacía con el nombre que prefieras (por ejemplo, `gestor_biblioteca`).

4. Ejecuta el archivo `DATABASE.sql` sobre esa base de datos. Puedes hacerlo de varias formas:
   - **Con pgAdmin**: haz clic derecho sobre tu base de datos > *Query Tool* > abre `DATABASE.sql` > presiona *Execute* (el rayo).
   - **Con psql** desde la terminal:
     ```bash
     psql -U tu_usuario -d gestor_biblioteca -f ruta/al/DATABASE.sql
     ```

5. Una vez ejecutado, las tablas necesarias como `usuarios`, `libros`, `prestamos`, etc., estarán listas para usarse.

6. Asegúrate de que las credenciales (usuario, contraseña y nombre de base de datos) coincidan con las usadas en tu código fuente.

---

Con eso, el sistema podrá conectarse a la base de datos sin problemas.


## 📬 Configuración de envío de correos

Para que el sistema pueda enviar correos electrónicos automáticos (por ejemplo, al registrar nuevos usuarios o enviar recordatorios antes de devolver un libro), sigue estos pasos:

1. **Descarga la carpeta `curl` incluida en el repositorio.**  
   Esta carpeta contiene todo lo necesario para enviar correos mediante comandos en segundo plano usando `curl`.

2. **No necesitas instalar nada adicional**, ya que el proyecto ya incluye los ejecutables necesarios.

3. Abre el archivo `.task` que se encuentra dentro del proyecto (o en la carpeta relacionada con el envío de correos).

4. **Actualiza las rutas absolutas** dentro del `.task` para que apunten correctamente a:
   - El ejecutable `curl.exe`
   - Los certificados incluidos (como `cacert.pem`)
   - El archivo batch o script que estés usando para enviar correos

5. Guarda los cambios y ejecuta el sistema.  
   Si las rutas están configuradas correctamente, los correos de bienvenida y recordatorio se enviarán de forma automática sin intervención adicional.

⚠️ **Importante:** Para el envío de correos se utiliza un servidor SMTP (como Gmail). Asegúrate de:
- Usar una cuenta válida de correo con contraseña de aplicación si tienes activada la verificación en dos pasos.
- Permitir conexiones SMTP desde aplicaciones externas si usas una cuenta común.

---

Con esto, tu sistema estará completamente funcional tanto en la parte visual como en la lógica de envío de correos.


## Contribuciones

Si deseas contribuir al proyecto, siéntete libre de hacer un **fork** y enviar un **pull request** con tus mejoras o correcciones. 

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/tobiasmeyhoefer/tobiasmeyhoefer/output/github-snake-dark.svg" />
  <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/tobiasmeyhoefer/tobiasmeyhoefer/output/github-snake.svg" />
  <img alt="github-snake" src="https://raw.githubusercontent.com/tobiasmeyhoefer/tobiasmeyhoefer/output/github-snake.svg" />
</picture>

###