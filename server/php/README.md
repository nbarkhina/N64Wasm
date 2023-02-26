# Cloud save states with PHP Backend

- unzip `dist` folder to document root
- unzip `server/php` to document root/api/
- enable sqlite3 extension in your php.ini
- Create password at bcrypt-generator.com or run `php -r "print password_hash('YOUR_PASSWORD', PASSWORD_BCRYPT);"` and replace BCRYPT_PASSWORD_HASH in index.php
- Update the CLOUDSAVEURL setting in your `settings.js` to "api"
- Place roms in document root/roms/ and update romlist.js
- Update `$baseurl` in `index.php` (if you serve from a subpath like /Home/n64wasm)
- Enjoy!
