function secsToMins(secs) {
    var remsecs = (secs%60 > 9 ? "" : "0") + secs%60
    var mins = Math.floor(secs/60)
    var hrs = Math.floor(mins/60)
    mins -= (hrs*60)
    return (hrs > 0 ? hrs + ":" + (mins%60 > 9 ? "" : "0") : "") + mins + ":" + remsecs
}
