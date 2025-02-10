import '../App.css'
import {Link} from 'react-router-dom'

function press(){
  console.log("Button Pressed");
}

function open_tab(url){
  window.open(url, '_blank').focus();
}

export function Home() {
  return (
    <>
      <div className="App">
        <title>I4.0 Project</title>
        <h1>I4.0 Project Dashboard</h1>
        <header className="App-header">
          <div className="App-grid" class="grid-container">
            <button className='App-button' onClick={() => press()}>{'MQTT'}</button>
            <button className='App-button' onClick={() => open_tab('http://192.168.10.2:1880')}>{'Node-Red'}</button>
            <button className='App-button' onClick={() => open_tab('http://192.168.10.2:8088/data/perspective/client/I4Project')}>{'Ignition'}</button>
            <Link to="/settings" className="App-link">
                <button className='App-button'>{'Settings'}</button>
            </Link>
          </div>
        </header>
      </div>
    </>
  )
}


