#include "web_server.h"
#include "esp_camera.h"
#include "servo_control.h"
#include "esp_http_server.h"

httpd_handle_t stream_httpd = NULL;
httpd_handle_t ctrl_httpd   = NULL;

static const char* INDEX_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Cam Control</title>
  <style>
    body {
      background:#111; color:#fff;
      font-family:sans-serif; text-align:center;
      margin:0; padding:10px;
    }
    img {
      width:100%; max-width:320px;
      border:2px solid #333;
      display:block; margin:10px auto;
    }
    .controls {
      display:flex; flex-direction:column;
      align-items:center; gap:10px;
      margin-top:15px;
    }
    .row { display:flex; gap:10px; }
    button {
      background:#1a1a1a; color:#fff;
      border:1px solid #444; border-radius:8px;
      font-size:16px; font-weight:bold;
      padding:18px 28px;
      cursor:pointer; touch-action:manipulation;
      min-width:90px;
    }
    button:active { background:#333; }
  </style>
</head>
<body>
  <h2>CAM CONTROL</h2>
  <img id="stream" />
  <div class="controls">
    <div class="row">
      <button onclick="send('TILT:UP')">UP</button>
    </div>
    <div class="row">
      <button onclick="send('PAN:LEFT')">LEFT</button>
      <button onclick="send('PAN:RIGHT')">RIGHT</button>
    </div>
    <div class="row">
      <button onclick="send('TILT:DOWN')">DOWN</button>
    </div>
    <div class="row">
      <button id="lightBtn" onclick="toggleLight()">Light On</button>
    </div>
    <div class="row">
      <button onclick="send('TRACKING:ON')">Tracking ON</button>
      <button onclick="send('TRACKING:OFF')">Tracking OFF</button>
    </div>
  </div>
  <div id="distance">Distance: --</div>
  <script>
    document.getElementById('stream').src =
      'http://' + window.location.hostname + ':81/stream';

    let lightOn = false;

    function send(cmd) {
      fetch('/cmd?action=' + cmd);
    }

    function toggleLight() {
      lightOn = !lightOn;
      send(lightOn ? 'LIGHT:ON' : 'LIGHT:OFF');
      document.getElementById('lightBtn').innerText = lightOn ? 'Light Off' : 'Light On';
    }

    setInterval(() => {
      fetch('/distance')
        .then(r => r.text())
        .then(d => document.getElementById('distance').innerText = 'Distance: ' + d + 'm');
    }, 500);
  </script>
</body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
  return ESP_OK;
}

static esp_err_t cmd_handler(httpd_req_t *req) {
  char buf[64];
  char action[32] = {0};
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    if (httpd_query_key_value(buf, "action", action, sizeof(action)) == ESP_OK) {
      handleServoCommand(action);
    }
  }
  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[64];
  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  if (res != ESP_OK) return res;
  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) { res = ESP_FAIL; break; }
    size_t hlen = snprintf(part_buf, sizeof(part_buf),
      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, "\r\n", 2);
    esp_camera_fb_return(fb);
    fb = NULL;
    if (res != ESP_OK) break;
    vTaskDelay(pdMS_TO_TICKS(15));
  }
  return res;
}
static esp_err_t distance_handler(httpd_req_t *req) {
  extern float lastDistance;
  char buf[16];
  snprintf(buf, sizeof(buf), "%.2f", lastDistance);
  httpd_resp_send(req, buf, strlen(buf));
  return ESP_OK;
}

void startServers() {
  httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
  stream_config.server_port = 81;
  stream_config.ctrl_port = 32769;
  httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
  if (httpd_start(&stream_httpd, &stream_config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.max_uri_handlers = 5;
  config.max_open_sockets = 5;
  httpd_uri_t index_uri = { .uri = "/",    .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL };
  httpd_uri_t cmd_uri   = { .uri = "/cmd", .method = HTTP_GET, .handler = cmd_handler,   .user_ctx = NULL };
  httpd_uri_t distance_uri = { .uri = "/distance", .method = HTTP_GET, .handler = distance_handler, .user_ctx = NULL };
  if (httpd_start(&ctrl_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(ctrl_httpd, &index_uri);
    httpd_register_uri_handler(ctrl_httpd, &cmd_uri);
    httpd_register_uri_handler(ctrl_httpd, &distance_uri);

  }
}

