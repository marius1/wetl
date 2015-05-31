var q;

+function ($) {
  'use strict';
  
  var TrafficLight = function (element, options) {
    this.$element  = $(element)
    this.options   = options
    this.state     = options.state || "red"
    
    var images = ['top', 'off', 'middle', 'off', 'bottom'];
    images[1] = (this.state == "red") ? "red" : "off";
    images[3] = (this.state == "green") ? "green" : "off";
    
    $.each(images, function(k, v) {
      $(element).append($('<div><img src="traffic-light/' + v + '.png" /></div>'));
    });    
  }
  
  TrafficLight.DEFAULTS = {
  }
  
  TrafficLight.prototype.setState = function (state) {    
    var setState = state || this.state;
        
    if (setState == "red") {
      this.$element.find('div:nth-child(2) img').replaceWith('<img src="traffic-light/red.png" />');
      this.$element.find('div:nth-child(4) img').replaceWith('<img src="traffic-light/off.png" />');
    } else {
      this.$element.find('div:nth-child(2) img').replaceWith('<img src="traffic-light/off.png" />');
      this.$element.find('div:nth-child(4) img').replaceWith('<img src="traffic-light/green.png" />');
    }
    
    this.state = (setState == "red") ? "green" : "red"
  }
  
  TrafficLight.prototype.toggle = function () {   
    var state = this.state;
    this.setState()
    
    var toggleEvent = $.Event('toggle')
    this.$element.trigger(toggleEvent, [ state ])    
  }
  
  function Plugin(option) {
    var args = arguments;
    return this.each(function () {
      var $this   = $(this)
      var data    = $this.data('bs.trafficlight')
      var options = typeof option == 'object' && option
      var state = (typeof option == 'object') ? options.state : args[1];

      if (!data) $this.data('bs.trafficlight', (data = new TrafficLight(this, options)))
      
      if (option == 'toggle') data.toggle()
      else if (option) data.setState(state)
    })
  }
  
  var old = $.fn.trafficLight

  $.fn.trafficLight             = Plugin
  $.fn.trafficLight.Constructor = TrafficLight
  
  $(document)
    .on('click.bs.trafficlight.data-api', 'div.traffic-light', function(e) {
      var $trafficLight = $(e.target)
      if (!$trafficLight.hasClass('traffic-light')) $trafficLight = $trafficLight.closest('.traffic-light')
      Plugin.call($trafficLight, 'toggle')
      e.preventDefault()
    })
  
  $(window).on('load', function () {
    $('div.traffic-light').each(function () {
      var $trafficlight = $(this)
      Plugin.call($trafficlight, $trafficlight.data())
    })
  })
}(jQuery);