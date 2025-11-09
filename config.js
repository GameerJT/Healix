// Healix Configuration File
// Replace these values with your actual API keys

const CONFIG = {
    // Firebase Configuration
    firebase: {
      apiKey: "AIzaSyBGHv2rhjDFpjFYrZKAo00CTk2HziaaMTM",
      authDomain: "clean-india-now.firebaseapp.com",
      authDomain: "clean-india-now.firebaseapp.com",
      projectId: "clean-india-now",
      storageBucket: "clean-india-now.appspot.com",
      messagingSenderId: "281358037895",
      appId: "1:281358037895:web:7ed32ec9a56cdff0a81922",
      measurementId: "G-R9YE34FZ2J"
    },

    // Groq API Configuration
    groq: {
        apiKey: "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",
        model: "llama-3.1-8b-instant",
        maxTokens: 500
    },

    // ESP32 BLE Configuration
    ble: {
        serviceUUID: "4fafc201-1fb5-459e-8fcc-c5c9c331914b",
        characteristics: {
            mq9: "beb5483e-36e1-4688-b7f5-ea07361b26a8",
            mq135: "beb5483e-36e1-4688-b7f5-ea07361b26a9",
            temperature: "beb5483e-36e1-4688-b7f5-ea07361b26aa",
            humidity: "beb5483e-36e1-4688-b7f5-ea07361b26ab"
        },
        deviceName: "SmartFit-Air-Monitor"
    },
    
    // ESP32 USB Configuration
    usb: {
        baudRate: 115200,
        dataFormat: "json",
        updateInterval: 2000,
        // Protocol: Send any character to start streaming
        // Data format: {"mq9":value,"mq135":value,"temperature":value,"humidity":value}
        protocol: "send-character-to-start"
    },

    // Food Recognition API (using Groq API for image recognition)
    foodApi: {
        apiKey: "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",  // Using Groq API key
        endpoint: "https://api.groq.com/openai/v1/chat/completions"
    },

    // ExerciseDB API Configuration (Free - no API key needed for basic usage)
    exerciseDB: {
        // Using free endpoints - no API key required
        endpoint: "https://exercisedb.p.rapidapi.com",
        // For production, get a free API key from: https://rapidapi.com/justin-WFnsXH_t6/api/exercisedb
        apiKey: "fb3df676dfmshd8d90c7ff9c1330p1bea54jsn5a662cc8b482",  // Optional: Add your RapidAPI key for higher limits
        useProxy: true  // Use CORS proxy for free tier
    },

    // AQICN API Configuration (Free API for real-time air quality data)
    aqicn: {
        // Get your free API key from: https://aqicn.org/data-platform/token/
        apiKey: "f4f42f11a05819096e37a20d7593e0600dea2268",  // Free tier: 1000 requests/day
        endpoint: "https://api.waqi.info/feed"
    }
};

// Export for use in other files
if (typeof module !== 'undefined' && module.exports) {
    module.exports = CONFIG;
}
