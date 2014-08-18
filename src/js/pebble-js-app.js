var mConfig = {};

Pebble.addEventListener("ready", function(e) {
	//console.log("91 Dub v2.0 is ready");
  loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
	Pebble.openURL(mConfig.configureUrl);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
    }
  }
);

function saveLocalData(config) {

  //console.log("loadLocalData() " + JSON.stringify(config));

  localStorage.setItem("fonttwo", parseInt(config.fonttwo));  
  loadLocalData();

}
function loadLocalData() {
  
	mConfig.fonttwo = parseInt(localStorage.getItem("fonttwo"));
	mConfig.configureUrl = "http://www.themapman.com/pebblewatch/simpleswapconfig.html";

	if(isNaN(mConfig.fonttwo)) {
		mConfig.fonttwo = 0;
	}
	


  //console.log("loadLocalData() " + JSON.stringify(mConfig));
}
function returnConfigToPebble() {
  //console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({
    "fonttwo":parseInt(mConfig.fonttwo), 
    
  });    
}