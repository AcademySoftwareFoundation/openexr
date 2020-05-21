 
// Scrolls down 90px from the top of 
// the document, to resize the navlist 
// padding and the title font-size 

var logoImage = document.getElementsByClassName("logo-img");
var headerElem = document.getElementsByClassName("collapse-header");
var headerImgElem = document.getElementsByClassName("header-logo");
var bannerElem = document.getElementsByClassName("scroll-banner");

window.onload = function() { 
	isScrolled() 
}; 

window.onscroll = function() { 
	scrollFunction() 
}; 
	
function isScrolled() { 
	if (document.body.scrollTop > 70 || 
		document.documentElement.scrollTop > 70) 
	{ 
		headerElem[0].style.height = "100px";
		bannerElem[0].style.height = "150px";
			  
		logoImage[0].style.height="60px"; 
		headerImgElem[0].style.marginLeft="50%";
		headerImgElem[0].style.left="0px";
		headerImgElem[0].style.top="20px";

	}  
	else { 
		headerElem[0].style.height = "200px"; 
		bannerElem[0].style.height = "250px";
					  
		logoImage[0].style.height="160px";
		headerImgElem[0].style.marginLeft="0%";
		headerImgElem[0].style.left="20px";
		headerImgElem[0].style.top="20px";
	} 
} 
	
function scrollFunction() { 
	if (document.body.scrollTop > 70 || 
		document.documentElement.scrollTop > 70) 
	{ 
		headerElem[0].style.height = "100px"; 
		bannerElem[0].style.height = "150px";
			  
		logoImage[0].style.height="60px"; 
		headerImgElem[0].style.marginLeft="50%";
		headerImgElem[0].style.left="0px";
		headerImgElem[0].style.top="20px";

	}  
	else { 
		headerElem[0].style.height = "200px"; 
		bannerElem[0].style.height = "250px";
					  
		logoImage[0].style.height="160px";
		headerImgElem[0].style.marginLeft="0%";
		headerImgElem[0].style.left="20px";
		headerImgElem[0].style.top="20px";
	} 
} 
