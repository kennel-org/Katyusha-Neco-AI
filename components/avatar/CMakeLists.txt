# Add m5stack-avatar as a component dependency
idf_component_register(
    SRCS "avatar.cpp"
    INCLUDE_DIRS ""
    REQUIRES 
        esp_lcd
        m5stack-avatar
        M5AtomS3
        M5GFX
)
