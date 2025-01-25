document.addEventListener('DOMContentLoaded', function() {
  const resetButton = document.querySelector('.reset-button');
  const viewStoredValues = document.getElementById('viewStoredValues');
  const voltageBtn = document.getElementById('voltageBtn');
  const currentBtn = document.getElementById('currentBtn');
  const maBtn = document.getElementById('maBtn');
  const uaBtn = document.getElementById('uaBtn');
  const modeButtons = document.getElementById('modeButtons');
  const currentOptions = document.getElementById('currentOptions');
  const backButton = document.querySelector('.back-button');
  const storeButton = document.querySelector('.store-button');

  function showNotification(message) {
    const notification = document.createElement('div');
    notification.className = 'notification';
    notification.style.background = '#ff4444'; 
    notification.style.color = '#ffffff'; 
    notification.textContent = message;
    document.body.appendChild(notification);

    setTimeout(() => {
      notification.style.animation = 'slideOut 0.5s ease-out';
      setTimeout(() => notification.remove(), 300);
    }, 5000);
  }

  function createStoredValuesView(data) {
    const values = data.split('\n').filter(v => v.trim());
    return `
      <div class="container stored-values-container">
        <div class="status-bar">
          <button class="reset-button">
            <div class="reset-icon"></div>
          </button>
        </div>
        <h1>Stored Values</h1>
        <div class="stored-values-grid">
          ${values.map(value => `
            <div class="value-card">
              <p>${value}</p>
            </div>
          `).join('')}
        </div>
        <button class="back-to-main" onclick="location.reload()">Back to Main</button>
      </div>
    `;
  }

  function attachEventListeners() {
    const resetButton = document.querySelector('.reset-button');
    resetButton.addEventListener('click', function() {
      fetch('/reset')
        .then(response => response.text())
        .then(data => {
          showNotification(data);
          location.reload();
        },5000);
    });
  }

  viewStoredValues.addEventListener('click', function() {
    fetch('/getStoredValues')
      .then(response => response.text())
      .then(data => {
        document.body.innerHTML = createStoredValuesView(data);
        attachEventListeners(); // Reattach event listeners after updating the HTML
      });
  });

  resetButton.addEventListener('click', function() {
    fetch('/reset')
      .then(response => response.text())
      .then(data => {
        showNotification(data);
        location.reload();
      },5000);
  });

  voltageBtn.addEventListener('click', function() {
    fetch('/toggle1')
      .then(response => response.text())
      .then(data => {
        document.getElementById('reading').textContent = data;
      });
  });

  currentBtn.addEventListener('click', function() {
    modeButtons.style.display = 'none';
    currentOptions.style.display = 'block';
  });

  maBtn.addEventListener('click', function() {
    fetch('/toggle2')
      .then(response => response.text())
      .then(data => {
        document.getElementById('reading').textContent = data;
      });
  });

  uaBtn.addEventListener('click', function() {
    fetch('/toggle3')
      .then(response => response.text())
      .then(data => {
        document.getElementById('reading').textContent = data;
      });
  });

  backButton.addEventListener('click', function() {
    currentOptions.style.display = 'none';
    modeButtons.style.display = 'flex';
  });

  storeButton.addEventListener('click', function() {
    fetch('/storeValues?store=true')
      .then(response => response.text())
      .then(data => {
        showNotification(data);
      });
  });

  function updateBatteryPercentage() {
    fetch('/getBatteryPercentage')
      .then(response => response.text())
      .then(percentage => {
        const batteryLevel = document.querySelector('.battery-level');
        const batteryPercentage = document.querySelector('.battery-percentage');
        batteryLevel.style.width = `${percentage}%`;
        batteryPercentage.textContent = `${percentage}%`;
      });
  }

  updateBatteryPercentage();
  setInterval(updateBatteryPercentage, 60000); 

  attachEventListeners();
});