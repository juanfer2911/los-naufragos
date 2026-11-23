CREATE TABLE IF NOT EXISTS planificaciones (
    id VARCHAR(36) PRIMARY KEY,
    propietario VARCHAR(255) NOT NULL,
    creado_en TIMESTAMP WITH TIME ZONE NOT NULL,
    resultado TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_planificaciones_propietario_fecha
    ON planificaciones (propietario, creado_en);
