CREATE TABLE IF NOT EXISTS conjuntos (
    conjunto_id VARCHAR(36) PRIMARY KEY,
    name VARCHAR(200) NOT NULL CHECK (length(trim(name)) > 0),
    description VARCHAR(2000),
    creador_sub VARCHAR(255) NOT NULL
);
-- Ampliacion compatible: no cambia las filas antiguas y puede repetirse al arrancar.
ALTER TABLE conjuntos ADD COLUMN IF NOT EXISTS quantum BIGINT CHECK (quantum >= 1);
CREATE TABLE IF NOT EXISTS procesos (
    conjunto_id VARCHAR(36) NOT NULL REFERENCES conjuntos(conjunto_id) ON DELETE CASCADE,
    posicion INTEGER NOT NULL CHECK (posicion >= 0),
    pid INTEGER NOT NULL CHECK (pid >= 0),
    arrival_time BIGINT NOT NULL CHECK (arrival_time >= 0),
    burst_time BIGINT NOT NULL CHECK (burst_time >= 1),
    priority INTEGER NOT NULL,
    PRIMARY KEY (conjunto_id, pid),
    UNIQUE (conjunto_id, posicion)
);
