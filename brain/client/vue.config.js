const fs = require('fs')

module.exports = {
	pwa: {
		name: 'StarTrack',
		themeColor: '',
		msTileColor: '',
		appleMobileWebAppCapable: 'yes',
		appleMobileWebAppStatusBarStyle: 'black',
		iconPaths: {
			faviconSVG: null,
			favicon32: null,
			favicon16: null,
			appleTouchIcon: null,
			maskIcon: null,
			msTileImage: null
		}
	},
    devServer: {
		allowedHosts: 'all',
        server: {
			type: 'https',
			options: {
				key: fs.readFileSync('../ssl/certificate.key').toString(),
				cert: fs.readFileSync('../ssl/certificate.crt').toString()
			}
        },
		proxy: {
			"/api": {
				"target": 'https://telescopi:5090',
				"pathRewrite": { '^/api': '' },
				"changeOrigin": true,
				"secure": true
			}
		},
	}
}


