-- setup_biblioteca.sql
-- Base de datos para sistema de biblioteca (PostgreSQL 15+)
-- Instrucciones:
--   1. Crear una BD: CREATE DATABASE biblioteca;
--   2. Ejecutar: psql -U tu_usuario -d biblioteca -f DATABASE.sql

BEGIN;

-- Tablas principales
CREATE TABLE usuarios (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(100) NOT NULL,  -- Contraseñas hasheadas (ejemplo: 'password1' = 10556473457816030167)
    email VARCHAR(100) UNIQUE,
    rol VARCHAR(20) DEFAULT 'Lector'
);

CREATE TABLE libros (
    id SERIAL PRIMARY KEY,
    titulo VARCHAR(255) NOT NULL,
    autor VARCHAR(255) NOT NULL,
    isbn VARCHAR(20) UNIQUE NOT NULL,
    editorial VARCHAR(255),
    año_publicacion INTEGER,
    materia VARCHAR(255),
    estado VARCHAR(20) DEFAULT 'Disponible'
);

CREATE TABLE prestamos (
    id SERIAL PRIMARY KEY,
    usuario_id INTEGER REFERENCES usuarios(id) ON DELETE CASCADE,
    libro_id INTEGER REFERENCES libros(id) ON DELETE CASCADE,
    fecha_prestamo DATE DEFAULT CURRENT_DATE,
    fecha_devolucion DATE,
    id_bibliotecario INTEGER REFERENCES usuarios(id),
    recordatorio_enviado BOOLEAN DEFAULT false
);

-- Datos iniciales (opcional)
INSERT INTO usuarios (username, password, email, rol) VALUES
    ('Admin', '18095434618418597693', 'Admin@gmail.com', 'Admin'),
    ('Bibliotecario', '10556473457816030167', 'bibliotecario@gmail.com', 'bibliotecario'),

INSERT INTO libros (titulo, autor, isbn, editorial, año_publicacion, materia) VALUES
    ('Cien años de soledad', 'Gabriel García Márquez', '978-0060883287', 'Editorial Planeta', 1967, 'Realismo mágico'),
    ('1984', 'George Orwell', '978-0452284234', 'Penguin Books', 1949, 'Distopía'),
    ('El Principito', 'Antoine de Saint-Exupéry', '978-8420709871', 'Salamandra', 1943, 'Filosofía');

-- Trigger para actualizar estado de libros (ejemplo)
CREATE OR REPLACE FUNCTION actualizar_estado_libro() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.fecha_devolucion IS NULL THEN
        UPDATE libros SET estado = 'Prestado' WHERE id = NEW.libro_id;
    ELSE
        UPDATE libros SET estado = 'Disponible' WHERE id = NEW.libro_id;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trig_actualizar_estado
AFTER INSERT OR UPDATE ON prestamos
FOR EACH ROW EXECUTE FUNCTION actualizar_estado_libro();

COMMIT;

-- Instrucción para probar (opcional):
-- INSERT INTO prestamos (usuario_id, libro_id, id_bibliotecario) VALUES (3, 1, 2);