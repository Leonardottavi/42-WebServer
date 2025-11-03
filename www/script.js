console.log("✅ JavaScript chargé ! FileHandler marche.");

function testJS() {
    document.getElementById("js-result").innerHTML = "✅ JavaScript work ! FileHandler manage correctly files .js";
    document.getElementById("js-result").style.color = "#4CAF50";
    document.getElementById("js-result").style.fontWeight = "bold";
    alert("✅ JS OK !");
}

window.onload = function() {
    console.log("✅ Page load, all static files are served !");
};