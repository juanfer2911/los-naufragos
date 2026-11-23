package com.losnaufragos.planificador;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.oauth2.core.*;
import org.springframework.security.oauth2.jwt.*;
import org.springframework.security.oauth2.server.resource.web.BearerTokenAuthenticationEntryPoint;
import org.springframework.security.web.SecurityFilterChain;
import org.springframework.web.cors.*;
import java.util.Arrays;
import java.util.List;

@Configuration
public class Seguridad {
    @Bean
    JwtDecoder jwtDecoder(@Value("${spring.security.oauth2.resourceserver.jwt.issuer-uri}") String issuer,
                          @Value("${spring.security.oauth2.resourceserver.jwt.jwk-set-uri}") String jwks,
                          @Value("${planificador.jwt.audience}") String audience) {
        NimbusJwtDecoder decoder = NimbusJwtDecoder.withJwkSetUri(jwks).build();
        OAuth2TokenValidator<Jwt> claims = jwt -> {
            if (jwt.getSubject() == null || jwt.getSubject().isBlank()
                    || jwt.getSubject().length() > 255 || jwt.getExpiresAt() == null
                    || !jwt.getAudience().contains(audience)) {
                return OAuth2TokenValidatorResult.failure(new OAuth2Error("invalid_token",
                        "El JWT requiere sub, exp y la audiencia del servicio.", null));
            }
            return OAuth2TokenValidatorResult.success();
        };
        decoder.setJwtValidator(new DelegatingOAuth2TokenValidator<>(
                JwtValidators.createDefaultWithIssuer(issuer), claims));
        return decoder;
    }

    @Bean
    SecurityFilterChain filtroSeguridad(HttpSecurity http, ObjectMapper json) throws Exception {
        var bearer = new BearerTokenAuthenticationEntryPoint();
        return http.csrf(csrf -> csrf.disable())
                .cors(cors -> { })
                .sessionManagement(s -> s.sessionCreationPolicy(SessionCreationPolicy.STATELESS))
                .authorizeHttpRequests(a -> a.anyRequest().authenticated())
                .oauth2ResourceServer(o -> o.jwt(jwt -> { }).authenticationEntryPoint((req, res, error) -> {
                    bearer.commence(req, res, error);
                    res.setContentType("application/json");
                    json.writeValue(res.getOutputStream(), new ApiModelos.Error("UNAUTHORIZED", "Se requiere un JWT valido."));
                }))
                .exceptionHandling(e -> e.authenticationEntryPoint((req, res, error) -> {
                    bearer.commence(req, res, error);
                    res.setContentType("application/json");
                    json.writeValue(res.getOutputStream(), new ApiModelos.Error("UNAUTHORIZED", "Se requiere un JWT valido."));
                }).accessDeniedHandler((req, res, error) -> {
                    res.setStatus(403);
                    res.setContentType("application/json");
                    json.writeValue(res.getOutputStream(), new ApiModelos.Error("FORBIDDEN", "Acceso denegado."));
                })).build();
    }

    @Bean
    CorsConfigurationSource corsConfigurationSource(@Value("${planificador.cors.origins}") String origenes) {
        var config = new CorsConfiguration();
        var permitidos = Arrays.stream(origenes.split(",")).map(String::trim).filter(s -> !s.isEmpty()).toList();
        if (permitidos.stream().anyMatch(s -> s.contains("*"))) {
            throw new IllegalArgumentException("CORS requiere origenes explicitos.");
        }
        config.setAllowedOrigins(permitidos);
        config.setAllowedMethods(List.of("GET", "POST", "OPTIONS"));
        config.setAllowedHeaders(List.of("Authorization", "Content-Type"));
        config.setAllowCredentials(false);
        var source = new UrlBasedCorsConfigurationSource();
        source.registerCorsConfiguration("/**", config);
        return source;
    }
}
