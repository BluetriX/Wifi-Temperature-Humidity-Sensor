
var options = {
  colors: ['#2E93fA', '#66DA26', '#546E7A', '#E91E63', '#FF9800', '#FF0000', '#ff4000', '#ff8000', '#ffbf00', '#ffff00', '#bfff00', '#00ff00'],
  
  chart: {
	type: 'line',
	stacked: false,
	height: '500px',
	zoom: {
	  type: 'x',
	  enabled: true,
	  autoScaleYaxis: true
	},
	toolbar: {
	  autoSelected: 'zoom'
	}
  },
  dataLabels: {
	enabled: false
  },
  series: temperaturen,
  markers: {
	size: 0,
  },
  title: {
	text: 'Temperaturen',
	align: 'left'
  },
  yaxis: {
	floating: false,
	title: {
	  text: 'Grad Celsius'
	},
  },
  xaxis: {
	type: 'datetime',
  },

  tooltip: {
	shared: false,
	x: {
	  show: true,
	  format: 'dd.MM.yy HH:mm',
	  formatter: undefined,
  },
  }
}

var chart = new ApexCharts(
  document.querySelector("#chart"),
  options
);

chart.render();


