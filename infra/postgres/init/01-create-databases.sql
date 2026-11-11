-- Create one database and login role per component.
-- Passwords are read from the local environment; no credentials are stored here.
\getenv keycloak_db_password KEYCLOAK_DB_PASSWORD
\getenv proceso_db_password PROCESO_DB_PASSWORD
\getenv planificador_db_password PLANIFICADOR_DB_PASSWORD

SELECT format('CREATE ROLE %I LOGIN PASSWORD %L', 'keycloak_user', :'keycloak_db_password') \gexec
SELECT format('CREATE ROLE %I LOGIN PASSWORD %L', 'proceso_user', :'proceso_db_password') \gexec
SELECT format('CREATE ROLE %I LOGIN PASSWORD %L', 'planificador_user', :'planificador_db_password') \gexec

CREATE DATABASE keycloak OWNER keycloak_user;
CREATE DATABASE proceso_db OWNER proceso_user;
CREATE DATABASE planificador_db OWNER planificador_user;
