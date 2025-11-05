// EcoWise Configuration File
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
    }
};

// Export for use in other files
if (typeof module !== 'undefined' && module.exports) {
    module.exports = CONFIG;
}
