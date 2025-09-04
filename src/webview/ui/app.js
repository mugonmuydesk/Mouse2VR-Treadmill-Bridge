// Extracted JavaScript from Mouse2VR WebView

let isRunning = false;
        let treadmillSpeedHistory = [];
        let gameSpeedHistory = [];
        let lastUpdateTime = Date.now();
        let currentDPI = 1000;  // Default DPI
        let sensitivity = 1.0;  // Current sensitivity multiplier
        
        // Initialize canvases when page loads
        window.addEventListener('DOMContentLoaded', () => {
            initializeStickCanvas();
            initializeSpeedCanvas();
            
            // Initialize DPI setting (default 1000)
            if (window.mouse2vr) {
                window.mouse2vr.setDPI(currentDPI);
            }
            
            // Start controller automatically since toggle is on by default
            if (window.mouse2vr) {
                toggleRunning();
            }
        });
        
        function toggleRunning() {
            const checkbox = document.getElementById('enableController');
            isRunning = checkbox.checked;
            
            if (isRunning) {
                window.mouse2vr.start();
                document.getElementById('statusValue').textContent = 'Running';
                document.getElementById('statusValue').style.color = '#0f7938';
            } else {
                window.mouse2vr.stop();
                document.getElementById('statusValue').textContent = 'Stopped';
                document.getElementById('statusValue').style.color = '#c42b1c';
            }
        }
        
        function updateSensitivityValue(value) {
            document.getElementById('sensitivityValue').textContent = parseFloat(value).toFixed(1);
        }
        
        function updateSensitivity(value) {
            updateSensitivityValue(value);
            sensitivity = parseFloat(value);
            if (window.mouse2vr) {
                window.mouse2vr.setSensitivity(parseFloat(value));
            }
        }

function updateDPI(value) {
            currentDPI = value;
            if (window.mouse2vr) {
                window.mouse2vr.setDPI(value);
            }
            // Disable custom input when preset is selected
            document.getElementById('customDPI').disabled = true;
        }
        
        function enableCustomDPI() {
            const customInput = document.getElementById('customDPI');
            customInput.disabled = false;
            customInput.focus();
            updateCustomDPI(customInput.value);
        }
        
        function updateCustomDPI(value) {
            currentDPI = parseInt(value);
            if (window.mouse2vr && !isNaN(currentDPI)) {
                window.mouse2vr.setDPI(currentDPI);
            }
        }
        
        function startMovementTest() {
            const button = document.getElementById('testButton');
            const status = document.getElementById('testStatus');
            const info = document.getElementById('testInfo');
            
            // Disable button and show status
            button.disabled = true;
            button.style.background = '#808080';
            button.style.cursor = 'not-allowed';
            status.textContent = 'Testing...';
            status.style.color = '#0078d4';
            info.style.display = 'block';
            
            // Send test command
            if (window.mouse2vr) {
                window.mouse2vr.startTest();
            }
            
            // Show countdown
            let secondsLeft = 5;
            const countdownInterval = setInterval(() => {
                secondsLeft--;
                if (secondsLeft > 0) {
                    status.textContent = `Testing... ${secondsLeft}s`;
                } else {
                    clearInterval(countdownInterval);
                    status.textContent = 'Test Complete - Check logs/debug.log';
                    status.style.color = '#10893e';
                    button.disabled = false;
                    button.style.background = '#0078d4';
                    button.style.cursor = 'pointer';
                    
                    // Hide info after a delay
                    setTimeout(() => {
                        info.style.display = 'none';
                        status.textContent = '';
                    }, 5000);
                }
            }, 1000);
        }
        
        function updateRate(value) {
            // Direct pass-through - no hidden mapping
            document.getElementById('targetRateValue').textContent = value + ' Hz';
            
            if (window.mouse2vr) {
                window.mouse2vr.setUpdateRate(value); // Pass directly
            }
        }
        
        function updateUIRefreshRate(value) {
            document.getElementById('uiRefreshRateValue').textContent = value + ' Hz';
            startPolling(value); // Update graph/metrics refresh rate
        }
        
        function updateAxisOptions() {
            const invertY = document.getElementById('invertY').checked;
            const lockX = document.getElementById('lockX').checked;
            
            // Send settings to backend
            if (window.mouse2vr) {
                window.mouse2vr.setInvertY(invertY);
                window.mouse2vr.setLockX(lockX);
            }
        }
        
        // Update speed and stick displays
        function updateSpeed(treadmillSpeed, gameSpeed, stickY, actualHz) {
            // Update treadmill speed value (show absolute for display)
            document.getElementById('treadmillSpeedValue').textContent = Math.abs(treadmillSpeed).toFixed(2) + ' m/s';
            
            // Update predicted game speed
            document.getElementById('gameSpeedValue').textContent = Math.abs(gameSpeed).toFixed(2) + ' m/s';
            
            // Use actual stick Y value if provided
            let stickPercent = 0;
            if (stickY !== undefined) {
                stickPercent = Math.abs(stickY) * 100;
            }
            document.getElementById('stickValue').textContent = Math.round(stickPercent) + '%';
            
            // Update achieved rate from backend
            if (actualHz !== undefined && actualHz > 0) {
                document.getElementById('achievedRateValue').textContent = actualHz + ' Hz';
            }
            
            // Update visualizations with actual stick position
            if (stickY !== undefined) {
                updateStickVisualization(stickY);
            }
            
            // Add both speeds to history (with sign for direction)
            addSpeedToHistory(treadmillSpeed, gameSpeed);
        }
        
        function initializeStickCanvas() {
            const canvas = document.getElementById('stickCanvas');
            if (!canvas) return;
            
            const ctx = canvas.getContext('2d');
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            
            drawStickBase(ctx, canvas.width, canvas.height);
        }
        
        function drawStickBase(ctx, w, h) {
            // Draw circle for stick area
            ctx.strokeStyle = '#616161';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.arc(w/2, h/2, Math.min(w, h) * 0.4, 0, Math.PI * 2);
            ctx.stroke();
            
            // Draw center dot
            ctx.fillStyle = '#0078d4';
            ctx.beginPath();
            ctx.arc(w/2, h/2, 5, 0, Math.PI * 2);
            ctx.fill();
        }
        
        function updateStickVisualization(value) {
            const canvas = document.getElementById('stickCanvas');
            if (!canvas) return;
            
            const ctx = canvas.getContext('2d');
            const w = canvas.width;
            const h = canvas.height;
            
            // Clear and redraw base
            ctx.clearRect(0, 0, w, h);
            drawStickBase(ctx, w, h);
            
            // Draw stick position (value ranges from -1 to 1)
            const radius = Math.min(w, h) * 0.4;
            const y = h/2 - (value * radius); // Negative = down, positive = up
            
            ctx.fillStyle = '#0078d4';
            ctx.beginPath();
            ctx.arc(w/2, y, 10, 0, Math.PI * 2);
            ctx.fill();
        }
        
        function initializeSpeedCanvas() {
            const canvas = document.getElementById('speedCanvas');
            if (!canvas) return;
            
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            
            // Initialize with empty history
            for (let i = 0; i < 50; i++) {
                treadmillSpeedHistory.push(0);
                gameSpeedHistory.push(0);
            }
        }
        
        function addSpeedToHistory(treadmillSpeed, gameSpeed) {
            treadmillSpeedHistory.push(treadmillSpeed);
            gameSpeedHistory.push(gameSpeed);
            
            if (treadmillSpeedHistory.length > 50) {
                treadmillSpeedHistory.shift();
            }
            if (gameSpeedHistory.length > 50) {
                gameSpeedHistory.shift();
            }
            
            drawSpeedGraph();
        }
        
        function drawSpeedGraph() {
            const canvas = document.getElementById('speedCanvas');
            if (!canvas) return;
            
            const ctx = canvas.getContext('2d');
            const w = canvas.width;
            const h = canvas.height;
            
            ctx.clearRect(0, 0, w, h);
            
            // Draw center line (zero speed)
            ctx.strokeStyle = '#d2d2d2';
            ctx.lineWidth = 1;
            ctx.setLineDash([2, 2]);
            ctx.beginPath();
            ctx.moveTo(0, h/2);
            ctx.lineTo(w, h/2);
            ctx.stroke();
            ctx.setLineDash([]);
            
            // Draw treadmill speed (physical)
            ctx.strokeStyle = '#0078d4'; // Microsoft Blue
            ctx.lineWidth = 2;
            ctx.beginPath();
            
            treadmillSpeedHistory.forEach((speed, i) => {
                const x = (i / (treadmillSpeedHistory.length - 1)) * w;
                const y = h/2 - ((speed / 2) * (h/2)); // Scale: 2 m/s = full height for treadmill
                
                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            });
            ctx.stroke();
            
            // Draw game speed (after sensitivity)
            ctx.strokeStyle = '#10893e'; // Microsoft Green
            ctx.lineWidth = 2;
            ctx.globalAlpha = 0.8; // Slight transparency
            ctx.beginPath();
            
            gameSpeedHistory.forEach((speed, i) => {
                const x = (i / (gameSpeedHistory.length - 1)) * w;
                const y = h/2 - ((speed / 6.1) * (h/2)); // Scale: 6.1 m/s = full height (HL2 max)
                
                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            });
            ctx.stroke();
            ctx.globalAlpha = 1.0;
            
            // Draw legend (Fluent Design style)
            const legendX = w - 150;
            const legendY = 10;
            
            // Semi-transparent background
            ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
            ctx.fillRect(legendX - 5, legendY - 5, 145, 45);
            
            // Legend items
            ctx.font = '12px "Segoe UI", sans-serif';
            
            // Treadmill speed
            ctx.strokeStyle = '#0078d4';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(legendX, legendY + 8);
            ctx.lineTo(legendX + 20, legendY + 8);
            ctx.stroke();
            
            ctx.fillStyle = '#1a1a1a';
            ctx.fillText('Treadmill Speed', legendX + 25, legendY + 12);
            
            // Game speed
            ctx.strokeStyle = '#10893e';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(legendX, legendY + 25);
            ctx.lineTo(legendX + 20, legendY + 25);
            ctx.stroke();
            
            ctx.fillStyle = '#1a1a1a';
            ctx.fillText('Game Speed (HL2)', legendX + 25, legendY + 29);
        }
        
        // Request speed updates periodically
        let updateInterval = null;
        
        function startPolling(rateHz = 60) {
            // Clear existing interval
            if (updateInterval) {
                clearInterval(updateInterval);
            }
            
            // Calculate interval from Hz (with minimum of 10ms)
            const intervalMs = Math.max(10, Math.floor(1000 / rateHz));
            
            updateInterval = setInterval(() => {
                if (window.mouse2vr && window.mouse2vr.getSpeed) {
                    window.mouse2vr.getSpeed();
                }
            }, intervalMs);
        }
        
        // Start with 5Hz polling by default
        let currentUIRefreshRate = 5;
        startPolling(currentUIRefreshRate);
        
        // Initialize
        window.addEventListener('DOMContentLoaded', () => {
            console.log('Mouse2VR UI loaded');
            if (window.mouse2vr) {
                window.mouse2vr.getStatus();
            }
        });

