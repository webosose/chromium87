// Listens for the app launching then creates the window
chrome.app.runtime.onLaunched.addListener(function() {
  runApp();
});


// Listens for the app restarting then re-creates the window.
chrome.app.runtime.onRestarted.addListener(function() {
  runApp();
});

// Creates the window for the application.
function runApp() {
  chrome.app.window.create('error_page_test.html', {
    id: "ErrorPageTest",
    innerBounds: {
      'width': 1024,
      'height': 768
    }
  });
}
