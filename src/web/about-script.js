/*
 * Author: fasion
 * Created time: 2022-02-03 16:09:04
 * Last Modified by: fasion
 * Last Modified time: 2022-02-03 16:12:29
 */

let $follow = document.querySelector('.follow');
let $fold = document.querySelector('.fold');
let $qrcode = document.querySelector('.qrcode-zone');

$follow.addEventListener('click', function() {
  $qrcode.style.display = 'block';
});

$fold.addEventListener('click', function() {
  $qrcode.style.display = 'none';
});
