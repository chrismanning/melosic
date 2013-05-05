function secsToMins(secs) {
    return Math.floor(secs/60) + ":" + (secs%60 > 9 ? "" : "0") + secs%60
}
