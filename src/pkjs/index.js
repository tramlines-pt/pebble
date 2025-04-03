var keys = require('message_keys');
var apiHost = "https://api.tramlines.de";
var radius = 5000;
var quickStartToggle = 0;
var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig);

var stationCache = {};
var stationIdCache;
var moreInfoCache = {};

Pebble.addEventListener("ready", function(e) {
  var tempRadius = localStorage.getItem("RADIUS");
  if (tempRadius) {
    radius = tempRadius;
  }
  var tempApiHost = localStorage.getItem("API_HOST");
  if (tempApiHost) {
    apiHost = tempApiHost;
  }
  var tempquickStartToggle = localStorage.getItem("QUICK_START");
  if (tempquickStartToggle) {
    quickStartToggle = tempquickStartToggle;
  }

  navigator.geolocation.getCurrentPosition(success, error, options);
});

Pebble.addEventListener("showConfiguration", function(e) {
  var url = clay.generateUrl();
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }

  // Get the keys and values from each config item
  var dict = clay.getSettings(e.response);
  radius = dict[keys.RADIUS] * 1000;
  localStorage.setItem("RADIUS", radius);
  apiHost = dict[keys.API_URL];
  localStorage.setItem("API_HOST", apiHost);
  console.log('radius: ' + radius);
  console.log('apiHost: ' + apiHost);
  quickStartToggle = dict[keys.QUICK_START_TOGGLE];
  localStorage.setItem("QUICK_START", quickStartToggle);
  console.log('quickStartToggle: ' + quickStartToggle);

  navigator.geolocation.getCurrentPosition(success, error, options);
});

function success(pos) {
    console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
    var lat = pos.coords.latitude;
    var lon = pos.coords.longitude;
    // overwrite with test data for either testing or for screenshots
    //lat = 50.934496;
    //lon = 6.981107;
    if (quickStartToggle == 1) {
      quickStart(lat, lon);
      return;
    }
    legacyStart(lat, lon);
}

function error(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

var options = {
  enableHighAccuracy: true,
  maximumAge: 10000,
  timeout: 10000
};

function quickStart(lat, lon) {
  var url = `${apiHost}/pebble/currentLocation?lat=${lat}&lon=${lon}&radius=${radius}`;
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.status >= 200 && req.status < 300) {
      var response = JSON.parse(req.responseText);
      stationCache = response.departures; // we always cache the last response, because we need it for another request
      stationIdCache = response.station[2];
      var departuresArray = response.departures.map(function(departure) {
        return [
        departure[2].toString(), // Line
        departure[3].toString(), // Destination
        formatTime(departure[4].toString()), // Time
        departure[5].toString() // Platform
      ];
      });
      // we need to check if the station data will fit in the buffer (uint32_t 4096 (Byte)), 
      // if not remove the last element until it fits
      while (JSON.stringify(departuresArray).length > 4000) {
        departuresArray.pop();
      }
      Pebble.sendAppMessage({"STATION_ARRAY": JSON.stringify(departuresArray)});
    } else {
      console.log('Error: ' + req.statusText);
      Pebble.sendAppMessage({"NO_INTERNET": 1});
    }
  };
  req.onerror = function() {
    Pebble.sendAppMessage({"NO_INTERNET": 1});
  };
  req.send();
}


function legacyStart(lat, lon) {
  var url = `${apiHost}/pebble/stations?lat=${lat}&lon=${lon}&radius=${radius}`;
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.status >= 200 && req.status < 300) {
      var response = JSON.parse(req.responseText);
      var stationsArray = response.map(function(station) {
        return [station[0], station[1].toString(), station[2].toString()];
      });
      // we need to check if the station data will fit in the buffer (uint32_t 4096) (it probably will), 
      // but if not remove the last element until it fit
      while (JSON.stringify(stationsArray).length > 4000) {
        stationsArray.pop();
      }
      Pebble.sendAppMessage({"STATIONS_ARRAY": JSON.stringify(stationsArray)});
    } else {
      console.log('Error: ' + req.statusText);
      Pebble.sendAppMessage({"NO_INTERNET": 1});
    }
  };
  req.onerror = function() {
    Pebble.sendAppMessage({"NO_INTERNET": 1});
  };
  req.send();
}

Pebble.addEventListener("appmessage", function(e) {
  var dict = e.payload;
  console.log('Received message: ' + JSON.stringify(dict));
  if (dict["GET_STATION"] || dict["GET_STATION_FROM_STOP"]) {
    var stationId = dict["GET_STATION"] || dict["GET_STATION_FROM_STOP"];
    var url = `${apiHost}/pebble/current/${stationId}`;
    var req = new XMLHttpRequest();
    req.open('GET', url, true);
    req.onload = function() {
      if (req.status >= 200 && req.status < 300) {
        var response = JSON.parse(req.responseText);
        stationCache = response; // we always cache the last response, because we need it for another request
        stationIdCache = stationId;
        var departuresArray = response.map(function(departure) {
          return [
          departure[2].toString(), // Line
          departure[3].toString(), // Destination
          formatTime(departure[4].toString()), // Time
          departure[5].toString() // Platform
        ];
        });
        // we need to check if the station data will fit in the buffer (uint32_t 4096 (Byte)), 
        // if not remove the last element until it fits
        while (JSON.stringify(departuresArray).length > 4000) {
          departuresArray.pop();
        }
        if (dict["GET_STATION"]) {
          Pebble.sendAppMessage({"STATION_ARRAY": JSON.stringify(departuresArray)});
        } else {
          Pebble.sendAppMessage({"STATION_FROM_STOP": JSON.stringify(departuresArray)});
        }
      } else {
        console.log('Error: ' + req.statusText);
        Pebble.sendAppMessage({"NO_INTERNET": 1});
      }
    };
    req.onerror = function() {
      Pebble.sendAppMessage({"NO_INTERNET": 1});
    };
    req.send();
  } else if (dict["GET_MORE_INFO"]) {
    // we get the uuid from the stationCache
    var uuid = stationCache[dict["GET_MORE_INFO"] - 1][0];
    var url = `${apiHost}/pebble/moreinfo/${stationIdCache}/${uuid}`;
    var req = new XMLHttpRequest();
    req.open('GET', url, true);
    req.onload = function() {
      if (req.status >= 200 && req.status < 300) {
        var response = JSON.parse(req.responseText);
        moreInfoCache = response;
        var moreInfoArray = [
          response.lineName,
          response.destination,
          response.platform.toString(),
          formatTime(response.timeDelayed),
          getDelayDifference(response.timeDelayed, response.timeSchedule).toString(),
          response.type,
        ];
        var stops = response.stops;
        Pebble.sendAppMessage({"MORE_INFO": JSON.stringify(moreInfoArray)});
        //console.log(JSON.stringify(stops));
        Pebble.sendAppMessage({"STOPS_MORE_INFO": JSON.stringify(stops)});
      } else if (req.status == 404) {
        // If we get a 404, that means the train has already left and there is no more info
        // In that case we send MORE_INFO_TIMEOUT with the value being the stationId
        Pebble.sendAppMessage({"MORE_INFO_TIMEOUT": stationIdCache});
      }
    };
    req.onerror = function() {
      Pebble.sendAppMessage({"NO_INTERNET": 1});
    };
    req.send();
  }
});

function formatTime(time) {
  // format is YYYY-MM-DDTHH:MM:SS+00:00
  var date = new Date(time);
  var hours = date.getHours();
  var minutes = date.getMinutes();
  if (minutes < 10) {
    minutes = '0' + minutes;
  }
  return hours + ':' + minutes;
}

function getDelayDifference(timeDelayed, timeSchedule) {
  // if the delay is negative, the train is early (very rare) (DB moment)
  // in that case add a '-' sign
  // if not, add a '+'
  // if the train is on time (diff = 0), we return an empty string
  
  var delayed = new Date(timeDelayed);
  var schedule = new Date(timeSchedule);
  var diff = delayed - schedule;
  if (diff == 0) {
    return '';
  }
  var sign = diff < 0 ? '-' : '+';
  diff = Math.abs(diff);
  var minutes = Math.floor(diff / 60000);
  return sign + minutes;
}