!function(t, r) {
    "object" == typeof exports && "undefined" != typeof module ? module.exports = r() : "function" == typeof define && define.amd ? define(r) : t.ES6Promise = r();
}(this, function() {
    "use strict";
    function t(t) {
        return "function" == typeof t;
    }
    var r = Array.isArray ? Array.isArray : function(t) {
        return "[object Array]" === Object.prototype.toString.call(t);
    }, e = 0, n = void 0, o = function(t, r) {
        i[e] = t, i[e + 1] = r, 2 === (e += 2) && (n ? n(s) : c());
    };
    var i = new Array(1e3);
    function s() {
        for (var t = 0; t < e; t += 2) {
            (0, i[t])(i[t + 1]), i[t] = void 0, i[t + 1] = void 0;
        }
        e = 0;
    }
    var u, c = (u = setTimeout, function() {
        return u(s, 1);
    });
    function a(t, r) {
        var e = this, n = new this.constructor(h);
        void 0 === n[f] && T(n);
        var i = e._state;
        if (i) {
            var s = arguments[i - 1];
            o(function() {
                return S(i, n, s, e._result);
            });
        } else E(e, n, t, r);
        return n;
    }
    function l(t) {
        if (t && "object" == typeof t && t.constructor === this) return t;
        var r = new this(h);
        return b(r, t), r;
    }
    var f = Math.random().toString(36).substring(2);
    function h() {}
    var _ = void 0, v = 1, p = 2, d = {
        error: null
    };
    function y(t) {
        try {
            return t.then;
        } catch (t) {
            return d.error = t, d;
        }
    }
    function m(r, e, n) {
        var i, s, u, c;
        e.constructor === r.constructor && n === a && e.constructor.resolve === l ? (u = r,
        (c = e)._state === v ? g(u, c._result) : c._state === p ? A(u, c._result) : E(c, void 0, function(t) {
            return b(u, t);
        }, function(t) {
            return A(u, t);
        })) : n === d ? (A(r, d.error), d.error = null) : void 0 === n ? g(r, e) : t(n) ? (i = e,
        s = n, o(function(t) {
            var r = !1, e = function(t, r, e, n) {
                try {
                    t.call(r, e, n);
                } catch (t) {
                    return t;
                }
            }(s, i, function(e) {
                r || (r = !0, i !== e ? b(t, e) : g(t, e));
            }, function(e) {
                r || (r = !0, A(t, e));
            }, t._label);
            !r && e && (r = !0, A(t, e));
        }, r)) : g(r, e);
    }
    function b(t, r) {
        var e, n;
        t === r ? A(t, new TypeError("You cannot resolve a promise with itself")) : (n = typeof (e = r),
        null === e || "object" !== n && "function" !== n ? g(t, r) : m(t, r, y(r)));
    }
    function w(t) {
        t._onerror && t._onerror(t._result), j(t);
    }
    function g(t, r) {
        t._state === _ && (t._result = r, t._state = v, 0 !== t._subscribers.length && o(j, t));
    }
    function A(t, r) {
        t._state === _ && (t._state = p, t._result = r, o(w, t));
    }
    function E(t, r, e, n) {
        var i = t._subscribers, s = i.length;
        t._onerror = null, i[s] = r, i[s + v] = e, i[s + p] = n, 0 === s && t._state && o(j, t);
    }
    function j(t) {
        var r = t._subscribers, e = t._state;
        if (0 !== r.length) {
            for (var n = void 0, o = void 0, i = t._result, s = 0; s < r.length; s += 3) n = r[s],
            o = r[s + e], n ? S(e, n, o, i) : o(i);
            t._subscribers.length = 0;
        }
    }
    function S(r, e, n, o) {
        var i = t(n), s = void 0, u = void 0, c = void 0, a = void 0;
        if (i) {
            if ((s = function(t, r) {
                try {
                    return t(r);
                } catch (t) {
                    return d.error = t, d;
                }
            }(n, o)) === d ? (a = !0, u = s.error, s.error = null) : c = !0, e === s) return void A(e, new TypeError("A promises callback cannot return that same promise."));
        } else s = o, c = !0;
        e._state !== _ || (i && c ? b(e, s) : a ? A(e, u) : r === v ? g(e, s) : r === p && A(e, s));
    }
    var P = 0;
    function T(t) {
        t[f] = P++, t._state = void 0, t._result = void 0, t._subscribers = [];
    }
    var Y = function() {
        function t(t, e) {
            this._instanceConstructor = t, this.promise = new t(h), this.promise[f] || T(this.promise),
            r(e) ? (this.length = e.length, this._remaining = e.length, this._result = new Array(this.length),
            0 === this.length ? g(this.promise, this._result) : (this.length = this.length || 0,
            this._enumerate(e), 0 === this._remaining && g(this.promise, this._result))) : A(this.promise, new Error("Array Methods must be provided an Array"));
        }
        return t.prototype._enumerate = function(t) {
            for (var r = 0; this._state === _ && r < t.length; r++) this._eachEntry(t[r], r);
        }, t.prototype._eachEntry = function(t, r) {
            var e = this._instanceConstructor, n = e.resolve;
            if (n === l) {
                var o = y(t);
                if (o === a && t._state !== _) this._settledAt(t._state, r, t._result); else if ("function" != typeof o) this._remaining--,
                this._result[r] = t; else if (e === x) {
                    var i = new e(h);
                    m(i, t, o), this._willSettleAt(i, r);
                } else this._willSettleAt(new e(function(r) {
                    return r(t);
                }), r);
            } else this._willSettleAt(n(t), r);
        }, t.prototype._settledAt = function(t, r, e) {
            var n = this.promise;
            n._state === _ && (this._remaining--, t === p ? A(n, e) : this._result[r] = e),
            0 === this._remaining && g(n, this._result);
        }, t.prototype._willSettleAt = function(t, r) {
            var e = this;
            E(t, void 0, function(t) {
                return e._settledAt(v, r, t);
            }, function(t) {
                return e._settledAt(p, r, t);
            });
        }, t;
    }();
    var x = function() {
        function t(r) {
            this[f] = P++, this._result = this._state = void 0, this._subscribers = [], h !== r && ("function" != typeof r && function() {
                throw new TypeError("You must pass a resolver function as the first argument to the promise constructor");
            }(), this instanceof t ? function(t, r) {
                try {
                    r(function(r) {
                        b(t, r);
                    }, function(r) {
                        A(t, r);
                    });
                } catch (r) {
                    A(t, r);
                }
            }(this, r) : function() {
                throw new TypeError("Failed to construct 'Promise': Please use the 'new' operator, this object constructor cannot be called as a function.");
            }());
        }
        return t.prototype.catch = function(t) {
            return this.then(null, t);
        }, t.prototype.finally = function(t) {
            var r = this.constructor;
            return this.then(function(e) {
                return r.resolve(t()).then(function() {
                    return e;
                });
            }, function(e) {
                return r.resolve(t()).then(function() {
                    throw e;
                });
            });
        }, t;
    }();
    return x.prototype.then = a, x.all = function(t) {
        return new Y(this, t).promise;
    }, x.race = function(t) {
        var e = this;
        return r(t) ? new e(function(r, n) {
            for (var o = t.length, i = 0; i < o; i++) e.resolve(t[i]).then(r, n);
        }) : new e(function(t, r) {
            return r(new TypeError("You must pass an array to race."));
        });
    }, x.resolve = l, x.reject = function(t) {
        var r = new this(h);
        return A(r, t), r;
    }, x._setScheduler = function(t) {
        n = t;
    }, x._setAsap = function(t) {
        o = t;
    }, x._asap = o, x.polyfill = function() {
        var t = void 0;
        if ("undefined" != typeof global) t = global; else if ("undefined" != typeof self) t = self; else try {
            t = Function("return this")();
        } catch (t) {
            throw new Error("polyfill failed because global object is unavailable in this environment");
        }
        delete t.Promise, t.Promise = x;
    }, x.Promise = x, x;
});