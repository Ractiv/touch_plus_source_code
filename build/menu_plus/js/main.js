/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

(function() {
  var $textFieldEmailAddress = $('.js-text-field-email-address');
  var $textFieldMessage = $('.js-text-field-message');
  var $buttonSend = $('.js-button-send');
  var $supportPage = $('.js-support-page');
  var $buttonViewData = $('.js-button-view-data');
  var $visualizePage = $('.js-visualize-page');
  var $toggleCircle = $('.js-toggle-circle');
  var $toggle0 = $('.js-toggle0');
  var $toggle1 = $('.js-toggle1');
  var $toggle2 = $('.js-toggle2');
  var $toggle3 = $('.js-toggle3');
  var $toggle4 = $('.js-toggle4');
  var $scrollBarCircle = $('.js-scroll-bar-circle');
  var $scrollBar = $('.js-scroll-bar');
  var $settingsPage = $('.js-settings-page');
  var $stage = $('.js-stage');
  var $buttonSupportDisabled = $('.js-button-support-disabled');
  var $buttonSupport = $('.js-button-support');
  var $buttonVisualizeDisabled = $('.js-button-visualize-disabled');
  var $buttonVisualize = $('.js-button-visualize');
  var $buttonSettingsDisabled = $('.js-button-settings-disabled');
  var $buttonSettings = $('.js-button-settings');
  var $menuBar = $('.js-menu-bar');
  var $downloadingPage = $('.js-downloading-page');
  var $calib0 = $('.js-calib0');
  var $calib1 = $('.js-calib1');
  var $calib2 = $('.js-calib2');
  var $calib3 = $('.js-calib3');
  var $calibCircle = $('.js-calib-circle');
  var $tutorialStage = $('.js-tutorial-stage');

  //----------------------------------------------------------------------------------------------------

  $("body").css("overflow", "hidden");

  $(document).ready(function()
  {
    $(window).resize(function()
    {
      window.resizeTo(640, 675);
      window.focus();
    });
  });

  //----------------------------------------------------------------------------------------------------

  var s3 = new S3();
  var updater = new Updater(ipc, s3);

  //----------------------------------------------------------------------------------------------------

  menu.append(new gui.MenuItem({ type: "normal", label: "Show control panel", click: function()
  {
    win.show();
    winShow = true;
  }}));

  menu.append(new gui.MenuItem({ type: "normal", label: "Update software", click: function()
  {
    updater.CheckForUpdate(true);
  }}));

  menu.append(new gui.MenuItem({ type: "normal", label: "Exit", click: function()
  {
    terminate();
  }}));

  tray.menu = menu;

  win.on("close", function()
  {
    win.hide();
    winShow = false;
  });

  win.on("minimize", function()
  {
    win.hide();
    winShow = false;
  });

  tray.on("click", function()
  {
    win.show();
    winShow = true;
  });

  //----------------------------------------------------------------------------------------------------

  ipc.MapFunction("show window", function(messageBody)
  {
    win.show();
    winShow = true;
  });

  ipc.MapFunction("hide window", function(messageBody)
  {
    win.hide();
    winShow = false;
  });

  ipc.MapFunction("show notification", function(messageBody)
  {
    var str_vec = messageBody.split(":");
    var notificationHead = str_vec[0];
    var notificationBody = str_vec[1];
    var notification = new Notification(notificationHead, { body: notificationBody });
  });

  ipc.MapFunction("exit", function(messageBody)
  {
    if (BlockExit)
      return;

    ipc.Clear();
    DeleteFolder(gui.App.dataPath);

    process.exit(0);
  });

  ipc.MapFunction("download failed", function(messageBody)
  {
    alert("Download failed, cannot connect to server");
    terminate();
  });

  ipc.MapFunction("download", function(messageBody)
  {
    var urlPath = messageBody.split("`");
    var url = urlPath[0];
    var path = urlPath[1];

    var http = require("http");
    var fs = require("fs");

    var file = fs.createWriteStream(path);
    var request = http.get(url, function(response)
    {
      response.pipe(file);
      ipc.SendMessage("track_plus", "download", "true");

    }).on("error", function(err)
    {
      alert("Download failed, cannot connect to server");
      ipc.SendMessage("track_plus", "download", "false");
    });
  });

  //----------------------------------------------------------------------------------------------------

  var currentPage = 0;

  var mouseX = 0;
  var mouseY = 0;
  var mouseDown = false;

  document.onmousemove = function(e)
  {
    mouseX = e.pageX;
    mouseY = e.pageY;
  };

  document.onmouseup = function(e)
  {
    mouseDown = false;
  };

  var animArray = [];
  var menuButtonArray = [];
  var scrollBarCircleArray = [];

  setToggle($toggle0, "toggleLaunchOnStartup");
  setToggle($toggle1, "togglePowerSavingMode");

  setToggle($toggle2, "toggleCheckForUpdates", function()
  {
    updater.Enabled = true;
    updater.CheckForUpdate();
  },
  function()
  {
    updater.Enabled = false;
  });

  setToggle($toggle3, "toggleTouchControl");

  ipc.GetResponse("daemon_plus", "get toggles", "", function(messageBody)
  {
    var toggleLaunchOnStartup = messageBody.substring(0, 1) == "1";
    var togglePowerSavingMode = messageBody.substring(1, 2) == "1";
    var toggleCheckForUpdates = messageBody.substring(2, 3) == "1";
    var toggleTouchControl = messageBody.substring(3, 4) == "1";
    var scrollBarAdjustClickHeightStep = parseInt(messageBody.substring(6, 7));

    switchToggle($toggle0, toggleLaunchOnStartup);
    switchToggle($toggle1, togglePowerSavingMode);
    switchToggle($toggle2, toggleCheckForUpdates);
    switchToggle($toggle3, toggleTouchControl);

    setScrollBar($scrollBar, 195, 431, 0, 9, scrollBarAdjustClickHeightStep, 6, "scrollBarAdjustClickHeightStep");
  });

  setMenuButton($buttonSettings, 0);
  setMenuButton($buttonVisualize, 1);
  setMenuButton($buttonSupport, 2);
  toggleMenuButton("buttonSettings");

  $buttonViewData.click(function()
  {
      ipc.SendMessage("track_plus", "toggle imshow", "");
  });

  $buttonSend.click(function()
  {
    var textFieldEmailAddress = getSelf($textFieldEmailAddress);
    var textFieldMessage = getSelf($textFieldMessage);

    var valid = true;
    if (textFieldEmailAddress.value.indexOf("@") === -1 ||
        textFieldEmailAddress.value.indexOf(".") === -1 ||
        textFieldMessage.value.indexOf(" ") === -1 ||
        textFieldMessage.value.length <= 10)
    {
      valid = false;
    }

    if (valid)
    {
      s3.GetKeys(function(keys)
      {
        var count = 0;
        
        for (var keyIndex in keys)
        {
          var key = keys[keyIndex];
          var keyPath = key.Key.split("/");

          if (keyPath[0] == "bug_report" && keyPath[1] == textFieldEmailAddress.value && keyPath[2].substring(0, 1) == "m")
            ++count;
        }

        s3.WriteTextKey("bug_report/" + textFieldEmailAddress.value + "/message" + count + ".txt", textFieldMessage.value, function()
        {
          textFieldEmailAddress.value = "";
          textFieldMessage.value = "";
          alert("Message sent");
        });
      },
      function()
      {
        alert("Server unreachable, please check your internet connection.");
      });
    }
    else
      alert("please make sure your email address and message are valid");
  });

  initStage();

  //----------------------------------------------------------------------------------------------------

  function resetCalibCircle()
  {
    calibCount = 0;
    calibCircle.targetTop = 297;
    calibCircle.currentTop = 297;
    calibCircle.targetLeft = 346;
    calibCircle.currentLeft = 346;
    calibCircle.initAnim = true;
  }
  resetCalibCircle();
  animArray.push(calibCircle);

  var calibArray = new Array();
  calibArray.push(calib0);
  calibArray.push(calib1);
  calibArray.push(calib2);
  calibArray.push(calib3);

  function setCalibActive(index)
  {
    for (var indexCalib in calibArray)
    {
      var calib = calibArray[indexCalib];
      if (indexCalib == index)
        calib.style.visibility = "visible";
      else
        calib.style.visibility = "hidden";
    }

    var circleTargetLeft;
    var circleTargetTop;

    if (index == 0 || index == 3)
      circleTargetLeft = 346;
    if (index == 1 || index == 2)
      circleTargetLeft = 508;
    if (index == 0 || index == 1)
      circleTargetTop = 297;
    if (index == 2 || index == 3)
      circleTargetTop = 420;

    calibCircle.targetTop = circleTargetTop;
    calibCircle.targetLeft = circleTargetLeft;
    calibCircle.initAnim = true;
  }

  setCalibActive(0);

  //----------------------------------------------------------------------------------------------------

  downloadingPage.targetTop = 675;
  downloadingPage.currentTop = 675;
  downloadingPage.initAnim = true;
  animArray.push(downloadingPage);

  tutorialStage.targetTop = 675;
  tutorialStage.currentTop = 675;
  tutorialStage.targetLeft = 0;
  tutorialStage.currentLeft = 0;
  tutorialStage.initAnim = true;
  animArray.push(tutorialStage);

  //----------------------------------------------------------------------------------------------------

  ipc.MapFunction("show download", function(messageBody)
  {
    downloadingPage.targetTop = 0;
    downloadingPage.initAnim = true;
    ipc.SendMessage("track_plus", "show download", "");
  });

  var calibCount = 0;
  var tutorialStageCount = 0;

  ipc.MapFunction("show welcome", function(messageBody)
  {
    tutorialStageCount = 0;
    tutorialStage.targetLeft = -640 * tutorialStageCount;
    tutorialStage.targetTop = 0;
    tutorialStage.initAnim = true;

    downloadingPage.targetTop = 675;
    downloadingPage.initAnim = true;
  });

  ipc.MapFunction("show wiggle", function(messageBody)
  {
    tutorialStageCount = 1;
    tutorialStage.targetLeft = -640 * tutorialStageCount;
    tutorialStage.targetTop = 0;
    tutorialStage.initAnim = true;
  });

  ipc.MapFunction("show point", function(messageBody)
  {
    tutorialStageCount = 2;
    tutorialStage.targetLeft = -640 * tutorialStageCount;
    tutorialStage.targetTop = 0;
    tutorialStage.initAnim = true;
  });

  ipc.MapFunction("show calibration", function(messageBody)
  {
    resetCalibCircle();
    tutorialStageCount = 3;
    tutorialStage.targetLeft = -640 * tutorialStageCount;
    tutorialStage.targetTop = 0;
    tutorialStage.initAnim = true;
  });

  ipc.MapFunction("show calibration next", function(messageBody)
  {
    if (calibCount < 3)
    {
      ++calibCount;
      setCalibActive(calibCount);
    }
  });

  ipc.MapFunction("show next", function(messageBody)
  {
    if (tutorialStageCount <= 3)
    {
      ++tutorialStageCount;
      tutorialStage.targetLeft = -640 * tutorialStageCount;
      tutorialStage.targetTop = 0;
      tutorialStage.initAnim = true;
    }
  });

  ipc.MapFunction("show stage", function(messageBody)
  {
    showStage();
  });

  function showStage()
  {
    tutorialStage.targetLeft = 0;
    tutorialStage.targetTop = 675;
    tutorialStage.initAnim = true;

    downloadingPage.targetLeft = 0;
    downloadingPage.targetTop = 675;
    downloadingPage.initAnim = true;

    win.hide();
    winShow = false;
  }

  //----------------------------------------------------------------------------------------------------

  setInterval(ipcThreadFunction, 100);
  function ipcThreadFunction()
  {
    ipc.Update();
  }

  setInterval(loop, 15);
  function loop()
  {
    if (winShow && !winShowOld)
    {
      toggleMenuButton("buttonSettings");
      setStage(0);
    }

    winShowOld = winShow;

    for (var i = 0; i < animArray.length; ++i)
    {
      if (animArray[i].initAnim)
      {
        animArray[i].initAnim = false;

        if (animArray[i].targetLeft != null && animArray[i].targetLeft != undefined)
          animArray[i].travelDistLeft = Math.abs(animArray[i].targetLeft - animArray[i].currentLeft);

        if (animArray[i].targetTop != null && animArray[i].targetTop != undefined)
          animArray[i].travelDistTop = Math.abs(animArray[i].targetTop - animArray[i].currentTop);
      }

      if (animArray[i].targetLeft != null && animArray[i].targetLeft != undefined)
      {
        var diffLeft = (animArray[i].targetLeft - animArray[i].currentLeft) * 0.2;
        var travelDistCapLeft = animArray[i].travelDistLeft * 0.05;
        if (diffLeft > travelDistCapLeft)
          diffLeft = travelDistCapLeft;
        else if (diffLeft < -travelDistCapLeft)
          diffLeft = -travelDistCapLeft;
          
        animArray[i].currentLeft += diffLeft;
        animArray[i].style.left = animArray[i].currentLeft + "px";
      }

      if (animArray[i].targetTop != null && animArray[i].targetTop != undefined)
      {
        var diffTop = (animArray[i].targetTop - animArray[i].currentTop) * 0.2;
        var travelDistCapTop = animArray[i].travelDistTop * 0.05;
        if (diffTop > travelDistCapTop)
          diffTop = travelDistCapTop;
        else if (diffTop < -travelDistCapTop)
          diffTop = -travelDistCapTop;
          
        animArray[i].currentTop += diffTop;
        animArray[i].style.top = animArray[i].currentTop + "px";

        if (animArray[i].currentTop > (675 - 1) && animArray[i].style.visibility != "hidden")
          animArray[i].style.visibility = "hidden";
        else if (animArray[i].currentTop < (675 - 1) && animArray[i].style.visibility != "visible")
          animArray[i].style.visibility = "visible";
      }
    }

    for (var i = 0; i < scrollBarCircleArray.length; ++i)
      if (scrollBarCircleArray[i].isDown && mouseDown)
      {
        var scrollBarCircle = scrollBarCircleArray[i];

        var xMin = scrollBarCircle.xMin;
        var xMax = scrollBarCircle.xMax;
        var stepMin = scrollBarCircle.stepMin;
        var stepMax = scrollBarCircle.stepMax;

        var posX = mouseX - scrollBarCircle.circleRadius;
        if (posX < xMin)
          posX = xMin;
        else if (posX > xMax)
          posX = xMax;

        scrollBarCircle.stepNew = Math.round(mapVal(posX, xMin, xMax, stepMin, stepMax)) + scrollBarCircle.offset;
        if (scrollBarCircle.stepNew != scrollBarCircle.stepOld)
          ipc.GetResponse("daemon_plus", "set toggle", scrollBarCircle.name + scrollBarCircle.stepNew, function(messageBody)
          {
            scrollBarCircle.style.left = mapVal(scrollBarCircle.stepNew - scrollBarCircle.offset, stepMin, stepMax, xMin, xMax) + "px";
          });

        scrollBarCircle.stepOld = scrollBarCircle.stepNew;
      }
  }

  function setToggle(toggleIn, toggleName, onCallback, offCallback)
  {
    var circle = getChild(toggleIn, "toggleCircle");
    circle.targetLeft = 0;
    circle.currentLeft = 0;
    circle.initAnim = true;

    circle.onCallback = onCallback;
    circle.offCallback = offCallback;

    animArray.push(circle);
    
    toggleIn.click(function()
    {
      if (circle.targetLeft == 0)
      {
        ipc.GetResponse("daemon_plus", "set toggle", toggleName + "1", function(messageBody)
        {
          switchToggle(toggleIn, true);
        });
      }
      else
      {
        ipc.GetResponse("daemon_plus", "set toggle", toggleName + "0", function(messageBody)
        {
          switchToggle(toggleIn, false);
        });
      }
    });
  }

  function switchToggle(toggleIn, on)
  {
    var circle = getChild(toggleIn, "toggleCircle");
    if (on)
    {
      circle.targetLeft = 25;
      circle.initAnim = true;
      if (circle.onCallback != null && circle.onCallback != undefined)
        circle.onCallback();
    }
    else
    {
      circle.targetLeft = 0;
      circle.initAnim = true;
      if (circle.offCallback != null && circle.offCallback != undefined)
        circle.offCallback();
    }
  }

  function setScrollBar(scrollBarIn, xMin, xMax, stepMin, stepMax, stepCurrent, circleRadius, name)
  {
    var circle = getChild(scrollBarIn, "scrollBarCircle");
    circle.isDown = false;
    circle.xMin = xMin;
    circle.xMax = xMax;
    circle.offset = -stepMin;
    circle.stepMin = stepMin;
    circle.stepMax = stepMax;
    circle.stepNew = circle.stepOld = stepCurrent + circle.offset;
    circle.circleRadius = circleRadius;
    circle.name = name;
    circle.style.left = mapVal(stepCurrent - circle.offset, stepMin, stepMax, xMin, xMax) + "px";
    
    circle.onmousedown = function()
    {
      circle.isDown = true;
      mouseDown = true;
    };

    circle.onmouseup = function()
    {
      circle.isDown = false;
      mouseDown = false;
    };

    circle.ondragstart = function(e)
    {
      e.preventDefault();
    };

    scrollBarCircleArray.push(circle);
  }

  function mapVal(value, leftMin, leftMax, rightMin, rightMax)
  {
    var leftSpan = leftMax - leftMin;
    var rightSpan = rightMax - rightMin;
    var valueScaled = (value - leftMin) / leftSpan;
    return rightMin + (valueScaled * rightSpan);
  }

  function initStage()
  {
    stage.targetLeft = 0;
    stage.currentLeft = 0;
    stage.initAnim = true;
    animArray.push(stage);
  }

  function setStage(pageNumIn)
  {
    if (pageNumIn != currentPage)
    {
      stage.targetLeft = -pageNumIn * 640;
      stage.initAnim = true;
      currentPage = pageNumIn;
    }
  }

  function setMenuButton(menuButton, pageNumIn)
  {
    var menuButtonEnabled = getSelf(menuButton);
    var menuButtonDisabled = getChild($menuBar, menuButtonEnabled.id + "Disabled");
    
    menuButtonEnabled.onclick = function()
    {
      toggleMenuButton(menuButtonEnabled.id);
      setStage(pageNumIn);
    };
    
    menuButtonDisabled.onclick = function()
    {
      toggleMenuButton(menuButtonEnabled.id);
      setStage(pageNumIn);
    };
    
    var menuButtonObject = new Object();
    menuButtonObject.id = menuButtonEnabled.id;
    menuButtonObject.menuButtonEnabled = menuButtonEnabled;
    menuButtonObject.menuButtonDisabled = menuButtonDisabled;
    
    menuButtonObject.enable = function()
    {
      menuButtonObject.menuButtonEnabled.style.visibility = "visible";
      menuButtonObject.menuButtonDisabled.style.visibility = "hidden";
    };
    
    menuButtonObject.disable = function()
    {
      menuButtonObject.menuButtonEnabled.style.visibility = "hidden";
      menuButtonObject.menuButtonDisabled.style.visibility = "visible";
    };
    
    menuButtonArray.push(menuButtonObject);
  }

  function toggleMenuButton(menuButtonId)
  {
    for (var i = 0; i < menuButtonArray.length; ++i)
      if (menuButtonArray[i].id != menuButtonId)
        menuButtonArray[i].disable();
      else
        menuButtonArray[i].enable();
  }

  function getSelf(selfJQuery)
  {
    var self;
    selfJQuery.each(function()
    {
      self = this;
    });
    return self;
  }

  function getChild(parent, childId)
  {
    var val;
    parent.find("*").each(function()
    {
      if (this.id == childId)
        val = this;
    });
    
    return val;
  }
})();