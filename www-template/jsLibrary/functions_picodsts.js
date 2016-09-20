// ---------------------------------------------------------

$(function() {
    $('#picoDstsv1').tree({
	data: picodstsv1DATA,
	autoEscape: false,
	autoOpen: false,
	closedIcon: '+',
	openedIcon: '- '
    });  
});

$(function() {
    $('#picoDstsv2').tree({
	data: picodstsv2DATA,
	autoEscape: false,
	autoOpen: false,
	closedIcon: '+',
	openedIcon: '- '
    });  
});

$(function() {
    $('#picoDstsv3').tree({
	data: picodstsv3DATA,
	autoEscape: false,
	autoOpen: 0,
	closedIcon: '+',
	openedIcon: '- '
    });  
});

// ---------------------------------------------------------
