CREATE TABLE prestamos (
    id SERIAL PRIMARY KEY,
    isbn VARCHAR(20),
    titulo VARCHAR(255),
    usuario VARCHAR(100),
    fecha_prestamo DATE,
    fecha_devolucion DATE
);

CREATE TABLE usuarios (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(100) NOT NULL,  -- Almacenaremos el hash, no la contraseña en texto plano
    email VARCHAR(100) UNIQUE,
    fecha_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE libros (
    id SERIAL PRIMARY KEY,
    titulo VARCHAR(255),
    autor VARCHAR(255),
    isbn VARCHAR(50) UNIQUE,
    editorial VARCHAR(255),
    año INT,
    materia VARCHAR(255),
    estado VARCHAR(20) DEFAULT 'Disponible'
);
